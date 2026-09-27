#ifndef REGS_GPIO_H
#define REGS_GPIO_H

#include <stdint.h>

// GPIO port A — addresses from RM0383
#define GPIOA_MODER (*(volatile uint32_t *)0x40020000)  // RM0383 §8.4.1, offset 0x00
#define GPIOA_ODR   (*(volatile uint32_t *)0x40020014)  // RM0383 §8.4.6, offset 0x14
#define GPIOA_AFRL  (*(volatile uint32_t *)0x40020020)  // RM0383 §8.4.9, offset 0x20 — alternate function, pins 0–7
#define GPIOA_AFRH  (*(volatile uint32_t *)0x40020024)  // RM0383 §8.4.10, offset 0x24 — alternate function, pins 8–15

#define PIN2  2   // PA2  = USART2_TX -> ST-LINK virtual COM port, UM1724 §6.8
#define PIN9  9   // PA9  = USART1_TX -> Mini RX (Arduino D8), DS10314 Table 8 / UM1724 Table 16
#define PIN10 10  // PA10 = USART1_RX <- Mini TX (Arduino D2), DS10314 Table 8 / UM1724 Table 16
#define PIN0  0   // PA0  = ADC1_IN0 <- mic module signal (Arduino A0), UM1724 Table 16
#define LED_PIN 5  // PA5 = LD2, User Manual UM1724 §7.6

#endif
