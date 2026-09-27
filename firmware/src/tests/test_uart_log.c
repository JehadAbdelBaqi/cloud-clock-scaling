#include "tests/test_uart_log.h"
#include "app_config.h"
#include "clock.h"
#if TEST_UART_LOG
#include "uart.h"
#endif

void test_uart_log_init(void) {
#if TEST_UART_LOG
    uart_init(UART_PC, clock_get_hz());               // PC serial monitor, via ST-LINK
    uart_puts(UART_PC, "test start: 16 MHz\r\n");
#endif
}

void test_uart_log_before_switch(void) {
#if TEST_UART_LOG
    uart_flush(UART_PC);
#endif
}

void test_uart_log_after_switch(clock_speed_t speed) {
#if TEST_UART_LOG
    uart_set_clock(UART_PC, clock_get_hz());          // new clock -> new BRR, or the text is garbage
    uart_puts(UART_PC, speed == CLOCK_50MHZ ? "50 MHz\r\n" : "16 MHz\r\n");
#else
    (void)speed;
#endif
}

void test_uart_log_line(const char *prefix, const char *line) {
#if TEST_UART_LOG
    uart_puts(UART_PC, prefix);
    uart_puts(UART_PC, line);
    uart_puts(UART_PC, "\r\n");
#else
    (void)prefix;
    (void)line;
#endif
}
