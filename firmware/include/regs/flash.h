#ifndef REGS_FLASH_H
#define REGS_FLASH_H

#include <stdint.h>

// Flash interface — address from RM0383
#define FLASH_ACR (*(volatile uint32_t *)0x40023C00)  // RM0383 §3.8.1, offset 0x00

#define FLASH_ACR_LATENCY_MASK (0xF << 0)  // LATENCY[3:0] — flash wait-states

#endif
