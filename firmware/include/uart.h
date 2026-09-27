#ifndef UART_H
#define UART_H

#include <stdint.h>

// USART2 TX-only on PA2 -> ST-LINK virtual COM port, 8N1 at UART_BAUD.

// pclk_hz = clock feeding USART2 (APB1). APB1 prescaler is left at /1, so = SYSCLK.
void uart_init(uint32_t pclk_hz);

// Recompute BRR after a clock change — same BRR on a new clock = wrong baud.
void uart_set_clock(uint32_t pclk_hz);

// Block until the last byte has fully left the pin. Call before a clock change.
void uart_flush(void);

void uart_puts(const char *s);

#endif
