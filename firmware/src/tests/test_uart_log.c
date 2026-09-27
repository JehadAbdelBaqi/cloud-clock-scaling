#include "tests/test_uart_log.h"
#include "app_config.h"
#include "clock.h"
#if TEST_UART_LOG
#include "uart.h"
#endif

void test_uart_log_init(void) {
#if TEST_UART_LOG
    uart_init(clock_get_hz());                        // PC serial monitor, via ST-LINK
    uart_puts("clock cycle test: 16 MHz\r\n");
#endif
}

void test_uart_log_before_switch(void) {
#if TEST_UART_LOG
    uart_flush();
#endif
}

void test_uart_log_after_switch(clock_speed_t speed) {
#if TEST_UART_LOG
    uart_set_clock(clock_get_hz());                   // new clock -> new BRR, or the text is garbage
    uart_puts(speed == CLOCK_50MHZ ? "50 MHz\r\n" : "16 MHz\r\n");
#else
    (void)speed;
#endif
}
