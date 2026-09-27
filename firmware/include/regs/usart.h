#ifndef REGS_USART_H
#define REGS_USART_H

#include <stdint.h>

// USART2 — addresses from RM0383 §19.6 (base 0x40004400)
#define USART2_SR  (*(volatile uint32_t *)0x40004400)  // status, offset 0x00
#define USART2_DR  (*(volatile uint32_t *)0x40004404)  // data, offset 0x04
#define USART2_BRR (*(volatile uint32_t *)0x40004408)  // baud rate, offset 0x08
#define USART2_CR1 (*(volatile uint32_t *)0x4000440C)  // control 1, offset 0x0C

#define USART_SR_TC   (1 << 6)   // transmission complete — last bit fully out of the pin
#define USART_SR_TXE  (1 << 7)   // data register empty — ready for the next byte
#define USART_CR1_TE  (1 << 3)   // transmitter enable
#define USART_CR1_UE  (1 << 13)  // USART enable

#endif
