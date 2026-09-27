#include "tests/cloud_cycle_test.h"

#include <stdint.h>

#include "app_config.h"
#include "clock.h"
#include "mic.h"
#include "uart.h"
#include "regs/systick.h"
#include "tests/test_led.h"
#include "tests/test_uart_log.h"

#define LINE_MAX 32  // longest line we accept, including the terminating 0

// ---- Small text helpers (no printf/sscanf — keeps the firmware small) ------

// Append v in decimal at line[*n].
static void put_uint(char *line, uint32_t *n, uint32_t v) {
    char tmp[10];
    uint32_t k = 0;
    do {
        tmp[k++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    while (k) {
        line[(*n)++] = tmp[--k];
    }
}

// Read a decimal number at *p and move *p past it. Returns 0 if there's no digit.
static int take_uint(const char **p, uint32_t *out) {
    const char *s = *p;
    uint32_t v = 0;
    if (*s < '0' || *s > '9') return 0;
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (uint32_t)(*s++ - '0');
    }
    *out = v;
    *p = s;
    return 1;
}

// ---- Bridge link (USART1) --------------------------------------------------

// Send "<tag>,<a>,<b>" + newline to the bridge, and echo it to the PC log.
static void send_line(char tag, uint32_t a, uint32_t b) {
    char line[LINE_MAX];
    uint32_t n = 0;
    line[n++] = tag;
    line[n++] = ',';
    put_uint(line, &n, a);
    line[n++] = ',';
    put_uint(line, &n, b);
    line[n] = '\0';

    uart_puts(UART_BRIDGE, line);
    uart_puts(UART_BRIDGE, "\n");
    test_uart_log_line("-> ", line);
}

// Collect bytes from the bridge. Returns the line once a newline arrives,
// else 0. Never waits.
static const char *read_line(void) {
    static char buf[LINE_MAX];
    static uint32_t len = 0;
    char c;

    while (uart_getc(UART_BRIDGE, &c)) {
        if (c == '\r') continue;  // accept \n or \r\n endings
        if (c == '\n') {
            buf[len] = '\0';
            len = 0;
            return buf;
        }
        if (len < LINE_MAX - 1) buf[len++] = c;  // too long: extra bytes dropped
    }
    return 0;
}

// "C,<seq>,<level>" -> 1 and the two numbers; anything else -> 0.
static int parse_command(const char *s, uint32_t *seq, uint32_t *level) {
    if (s[0] != 'C' || s[1] != ',') return 0;
    s += 2;
    if (!take_uint(&s, seq) || *s++ != ',') return 0;
    if (!take_uint(&s, level)) return 0;
    return *s == '\0';
}

// ---- Clock ------------------------------------------------------------------

// Switch the clock with both UARTs kept on the right baud.
static void switch_to(clock_speed_t speed) {
    test_uart_log_before_switch();                 // PC log: last byte out at the old baud
    uart_flush(UART_BRIDGE);
    clock_set(speed);
    uart_set_clock(UART_BRIDGE, clock_get_hz());
    test_uart_log_after_switch(speed);             // PC log: new BRR, print the speed
}

// How long one SysTick countdown takes at the current clock, in ms.
// Used to count down the hold — coarse (one countdown), fine for this test.
static uint32_t countdown_ms(void) {
    uint64_t ticks = (uint64_t)BLINK_RELOAD + 1;
    uint32_t hz = clock_get_hz() / (SYSTICK_DIV8 ? 8 : 1);
    return (uint32_t)(ticks * 1000 / hz);
}

// ---- Test -------------------------------------------------------------------

void cloud_cycle_test_run(void) {
    clock_speed_t speed = BOOT_CLOCK;   // main() already put us here at boot
    uint32_t seq = 0;                   // reading number
    uint32_t hold_left_ms = 0;          // 0 = not holding a raised clock
    uint32_t hold_seq = 0;              // seq of the command we're holding for

    uart_init(UART_BRIDGE, clock_get_hz());
    mic_init();
    test_uart_log_init();

    uint32_t loudest = 0;               // loudest mic window since the last reading
    int clap_sent = 0;                  // 1 = already sent a clap this countdown
    uint32_t since_reading_ms = 0;      // time since the last regular reading

    while (1) {
        test_led_loop();

        // Mic: one sample per pass; a finished window gives its loudness
        uint32_t p2p;
        if (mic_poll(&p2p)) {
            if (p2p > loudest) loudest = p2p;

            // Clap: send straight away, don't wait for the next countdown.
            // Once per countdown, so a long noise can't flood the cloud.
            if (p2p >= MIC_CLAP_LEVEL && !clap_sent) {
                clap_sent = 1;
                loudest = 0;            // counted — don't send it again below
                send_line('S', ++seq, p2p);
            }
        }

        // Command in from the cloud (via the bridge)
        const char *line = read_line();
        if (line) {
            test_uart_log_line("<- ", line);

            uint32_t cseq, level;
            if (parse_command(line, &cseq, &level) && level > 0) {
                // Level 1+ = 50 MHz (100 MHz isn't supported yet)
                if (speed != CLOCK_50MHZ) {
                    speed = CLOCK_50MHZ;
                    switch_to(speed);
                }
                hold_left_ms = HOLD_S * 1000;   // start, or restart, the hold
                hold_seq = cseq;
                send_line('A', cseq, 50);
            }
        }

        // Every SysTick countdown: count down the hold and the reading interval
        if (STK_CTRL & STK_CTRL_COUNTFLAG) {
            test_led_on_switch();
            uint32_t elapsed_ms = countdown_ms();  // at the clock this countdown ran on

            if (hold_left_ms) {
                if (hold_left_ms <= elapsed_ms) {
                    // Hold over: drop back by ourselves — never waits on the cloud
                    hold_left_ms = 0;
                    speed = CLOCK_16MHZ;
                    switch_to(speed);
                    send_line('A', hold_seq, 16);
                } else {
                    hold_left_ms -= elapsed_ms;
                }
            }

            clap_sent = 0;  // one clap per countdown

            // Regular reading every READING_INTERVAL_S: the loudest window since
            // the last one. Each reading runs the Lambda, so not too often.
            since_reading_ms += elapsed_ms;
            if (since_reading_ms >= READING_INTERVAL_S * 1000) {
                since_reading_ms = 0;
                send_line('S', ++seq, loudest);
                loudest = 0;
            }
        }
    }
}
