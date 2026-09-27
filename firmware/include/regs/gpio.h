#ifndef REGS_GPIO_H
#define REGS_GPIO_H

#include <stdint.h>

// GPIO port A — addresses from RM0383
#define GPIOA_MODER (*(volatile uint32_t *)0x40020000)  // RM0383 §8.4.1, offset 0x00
#define GPIOA_ODR   (*(volatile uint32_t *)0x40020014)  // RM0383 §8.4.6, offset 0x14

#define PIN5 5  // PA5 = LD2, User Manual UM1724 §7.6

#endif
