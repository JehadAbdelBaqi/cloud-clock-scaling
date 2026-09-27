#ifndef UART_H
#define UART_H

#include <stdint.h>

// Polled UART driver, 8N1 at UART_BAUD. One driver for every port — each port's
// USART, pins and clock enable come from the table in uart_config.h.
typedef enum {
    UART_PC     = 0,  // USART2, PA2 TX only -> ST-LINK virtual COM port (local test logging)
    UART_BRIDGE = 1,  // USART1, PA9 TX / PA10 RX <-> Genesis Mini (uart-mqtt-bridge)
} uart_port_t;

// pclk_hz = clock feeding the USART. APB1/APB2 prescalers are left at /1, so = SYSCLK.
void uart_init(uart_port_t port, uint32_t pclk_hz);

// Recompute BRR after a clock change — same BRR on a new clock = wrong baud.
void uart_set_clock(uart_port_t port, uint32_t pclk_hz);

// Block until the last byte has fully left the pin. Call before a clock change.
void uart_flush(uart_port_t port);

void uart_puts(uart_port_t port, const char *s);

// Non-blocking: if a byte has arrived, put it in *c and return 1; else return 0.
int uart_getc(uart_port_t port, char *c);

#endif
