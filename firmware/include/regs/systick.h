#ifndef REGS_SYSTICK_H
#define REGS_SYSTICK_H

#include <stdint.h>

// SysTick is part of the Cortex-M4 core — addresses from PM0214 §4.5
#define STK_CTRL   (*(volatile uint32_t *)0xE000E010)  // SysTick Control and Status Register
#define STK_LOAD   (*(volatile uint32_t *)0xE000E014)  // SysTick Reload Value Register

#define STK_CTRL_ENABLE    (1 << 0)   // ENABLE bit
#define STK_CTRL_CLKSOURCE (1 << 2)   // CLKSOURCE bit (1 = processor clock/AHB)
#define STK_CTRL_COUNTFLAG (1 << 16)  // COUNTFLAG bit — set when the count reaches 0

#endif
