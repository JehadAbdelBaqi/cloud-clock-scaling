#ifndef REGS_GPIO_H
#define REGS_GPIO_H

#include <stdint.h>

// GPIO port A — addresses from RM0383
#define GPIOA_MODER (*(volatile uint32_t *)0x40020000)  // RM0383 §8.4.1, offset 0x00
#define GPIOA_ODR   (*(volatile uint32_t *)0x40020014)  // RM0383 §8.4.6, offset 0x14
#define GPIOA_AFRL  (*(volatile uint32_t *)0x40020020)  // RM0383 §8.4.9, offset 0x20 — alternate function, pins 0–7

#define PIN2 2  // PA2 = USART2_TX -> ST-LINK virtual COM port, UM1724 §6.8
#define LED_PIN 5  // PA5 = LD2, User Manual UM1724 §7.6

#endif
