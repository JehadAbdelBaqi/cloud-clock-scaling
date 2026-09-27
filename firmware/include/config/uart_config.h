#ifndef UART_CONFIG_H
#define UART_CONFIG_H

#include <stdint.h>

#include "regs/gpio.h"
#include "regs/rcc.h"
#include "regs/usart.h"
#include "drivers/uart.h"

// Settings for each UART port — used by uart.c. All pins are on port A, AF7.
typedef struct {
    usart_regs_t      *usart;
    volatile uint32_t *rcc_enr;   // clock-enable register (USART1 is on APB2, USART2 on APB1)
    uint32_t           rcc_bit;
    uint8_t            tx_pin;
    uint8_t            rx_pin;
    uint8_t            use_rx;    // 0 = send only
} uart_cfg_t;

static const uart_cfg_t uart_cfg[] = {
    [UART_PC]     = { USART2, &RCC_APB1ENR, USART2EN, PIN2, 0,     0 },
    [UART_BRIDGE] = { USART1, &RCC_APB2ENR, USART1EN, PIN9, PIN10, 1 },
};

#endif
