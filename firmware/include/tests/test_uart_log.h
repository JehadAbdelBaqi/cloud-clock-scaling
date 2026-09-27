#ifndef TESTS_TEST_UART_LOG_H
#define TESTS_TEST_UART_LOG_H

#include "clock.h"

// PC serial logging for the tests. Does nothing unless TEST_UART_LOG = 1.

// Set up USART2 at the current clock and print the start line.
void test_uart_log_init(void);

// Call just before clock_set() — let the last byte finish at the old baud.
void test_uart_log_before_switch(void);

// Call just after clock_set() — new BRR for the new clock, then print the speed.
void test_uart_log_after_switch(clock_speed_t speed);

// Print prefix + line + newline (e.g. "-> " + a line sent to the bridge).
void test_uart_log_line(const char *prefix, const char *line);

#endif
