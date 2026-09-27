#ifndef STM32F411_REGISTERS_H
#define STM32F411_REGISTERS_H

#include <stdint.h>

// SysTick is part of the Cortex-M4 core — addresses from PM0214 §4.5
#define STK_CTRL   (*(volatile uint32_t *)0xE000E010)  // SysTick Control and Status Register
#define STK_LOAD   (*(volatile uint32_t *)0xE000E014)  // SysTick Reload Value Register

// STM32F411 peripherals — addresses from RM0383
#define RCC_AHB1ENR (*(volatile uint32_t *)0x40023830)  // RM0383 §6.3.9, offset 0x30
#define GPIOA_MODER (*(volatile uint32_t *)0x40020000)  // RM0383 §8.4.1, offset 0x00
#define GPIOA_ODR   (*(volatile uint32_t *)0x40020014)  // RM0383 §8.4.6, offset 0x14

#define STK_CTRL_ENABLE (1 << 0)  // SysTick Control and Status Register: ENABLE bit
#define STK_CTRL_CLKSOURCE (1 << 2)  // SysTick Control and Status Register: CLKSOURCE bit (1 = processor clock/AHB)
#define STK_CTRL_COUNTFLAG (1 << 16)  // SysTick Control and Status Register: COUNTFLAG bit
#define STK_LOAD_HEX_VALUE 0x7FFFFF  // Reload value for SysTick (24-bit, half of max) — never changed, so blink rate tracks the CPU clock

#define GPIOAEN (1 << 0)   // RCC_AHB1ENR bit 0
#define PIN5    5          // PA5 = LD2, User Manual UM1724 §7.6

#endif
