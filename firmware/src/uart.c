#include <stdint.h>

#include "drivers/uart.h"
#include "config/uart_config.h"
#include "config/app_config.h"
#include "regs/gpio.h"
#include "regs/rcc.h"
#include "regs/usart.h"

#define GPIO_AF_USART 0x7  // AF7 = USART1/USART2 (DS10314 alternate function table)

// Pin -> alternate function mode, AF7. AFRL holds pins 0–7, AFRH pins 8–15.
static void pin_to_usart(uint8_t pin) {
    GPIOA_MODER = (GPIOA_MODER & ~(0x3 << (pin * 2))) | (0x2 << (pin * 2));

    if (pin < 8) {
        GPIOA_AFRL = (GPIOA_AFRL & ~(0xF << (pin * 4))) | (GPIO_AF_USART << (pin * 4));
    } else {
        uint8_t n = pin - 8;
        GPIOA_AFRH = (GPIOA_AFRH & ~(0xF << (n * 4))) | (GPIO_AF_USART << (n * 4));
    }
}

void uart_set_clock(uart_port_t port, uint32_t pclk_hz) {
    // OVER8 = 0: BRR = pclk / baud, rounded (16 MHz -> 139, 50 MHz -> 434)
    uart_cfg[port].usart->BRR = (pclk_hz + UART_BAUD / 2) / UART_BAUD;
}

void uart_init(uart_port_t port, uint32_t pclk_hz) {
    const uart_cfg_t *cfg = &uart_cfg[port];

    // 1. Clocks: GPIOA (pins) and the USART itself
    RCC_AHB1ENR |= GPIOAEN;
    *cfg->rcc_enr |= cfg->rcc_bit;

    // 2. Pins: hand them to the USART
    pin_to_usart(cfg->tx_pin);
    if (cfg->use_rx) {
        pin_to_usart(cfg->rx_pin);
    }

    // 3. Baud
    uart_set_clock(port, pclk_hz);

    // 4. USART on, transmitter (+ receiver) on. 8 data bits, no parity, 1 stop = reset defaults
    cfg->usart->CR1 = USART_CR1_UE | USART_CR1_TE | (cfg->use_rx ? USART_CR1_RE : 0);
}

void uart_flush(uart_port_t port) {
    while (!(uart_cfg[port].usart->SR & USART_SR_TC)) {}
}

void uart_puts(uart_port_t port, const char *s) {
    usart_regs_t *u = uart_cfg[port].usart;
    while (*s) {
        while (!(u->SR & USART_SR_TXE)) {}  // wait for room
        u->DR = (uint8_t)*s++;
    }
}

int uart_getc(uart_port_t port, char *c) {
    usart_regs_t *u = uart_cfg[port].usart;
    uint32_t sr = u->SR;

    if (!(sr & (USART_SR_RXNE | USART_SR_ORE))) {
        return 0;                            // nothing arrived
    }
    // Reading DR takes the byte and clears RXNE. After SR, it also clears an
    // overrun (ORE) — otherwise ORE would stay set and block further receiving.
    *c = (char)(u->DR & 0xFF);
    return (sr & USART_SR_RXNE) ? 1 : 0;
}
