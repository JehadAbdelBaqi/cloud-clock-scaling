#ifndef REGS_USART_H
#define REGS_USART_H

#include <stdint.h>

// Every USART has the same register layout, just at a different base address
// (RM0383 §19.6). So one struct describes them all.
typedef struct {
    volatile uint32_t SR;    // status, offset 0x00
    volatile uint32_t DR;    // data, offset 0x04
    volatile uint32_t BRR;   // baud rate, offset 0x08
    volatile uint32_t CR1;   // control 1, offset 0x0C
    volatile uint32_t CR2;   // control 2, offset 0x10
    volatile uint32_t CR3;   // control 3, offset 0x14
    volatile uint32_t GTPR;  // guard time + prescaler, offset 0x18
} usart_regs_t;

// Base addresses — RM0383 memory map (§2.3)
#define USART1 ((usart_regs_t *)0x40011000)  // APB2
#define USART2 ((usart_regs_t *)0x40004400)  // APB1

#define USART_SR_ORE  (1 << 3)   // overrun — a byte arrived before the last one was read
#define USART_SR_RXNE (1 << 5)   // a received byte is waiting in DR
#define USART_SR_TC   (1 << 6)   // transmission complete — last bit fully out of the pin
#define USART_SR_TXE  (1 << 7)   // data register empty — ready for the next byte
#define USART_CR1_RE  (1 << 2)   // receiver enable
#define USART_CR1_TE  (1 << 3)   // transmitter enable
#define USART_CR1_UE  (1 << 13)  // USART enable

#endif
