#include <stdint.h>

#include "uart.h"
#include "app_config.h"
#include "regs/gpio.h"
#include "regs/rcc.h"
#include "regs/usart.h"

void uart_set_clock(uint32_t pclk_hz) {
    // OVER8 = 0: BRR = pclk / baud, rounded (16 MHz -> 139, 50 MHz -> 434)
    USART2_BRR = (pclk_hz + UART_BAUD / 2) / UART_BAUD;
}

void uart_init(uint32_t pclk_hz) {
    // 1. Clocks: GPIOA (for PA2) and USART2
    RCC_AHB1ENR |= GPIOAEN;
    RCC_APB1ENR |= USART2EN;

    // 2. Pin: PA2 mode = 10 (alternate function), AF7 = USART2
    GPIOA_MODER = (GPIOA_MODER & ~(0x3 << (PIN2 * 2))) | (0x2 << (PIN2 * 2));
    GPIOA_AFRL  = (GPIOA_AFRL  & ~(0xF << (PIN2 * 4))) | (0x7 << (PIN2 * 4));

    // 3. Baud
    uart_set_clock(pclk_hz);

    // 4. USART on, transmitter only (8 data bits, no parity, 1 stop = reset defaults)
    USART2_CR1 = USART_CR1_UE | USART_CR1_TE;
}

void uart_flush(void) {
    while (!(USART2_SR & USART_SR_TC)) {}
}

void uart_puts(const char *s) {
    while (*s) {
        while (!(USART2_SR & USART_SR_TXE)) {}                // wait for room
        USART2_DR = (uint8_t)*s++;
    }
}
