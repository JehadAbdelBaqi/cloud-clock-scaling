#ifndef REGS_RCC_H
#define REGS_RCC_H

#include <stdint.h>

// Reset and clock control — addresses from RM0383
#define RCC_CR      (*(volatile uint32_t *)0x40023800)  // RM0383 §6.3.1, offset 0x00
#define RCC_PLLCFGR (*(volatile uint32_t *)0x40023804)  // RM0383 §6.3.2, offset 0x04
#define RCC_CFGR    (*(volatile uint32_t *)0x40023808)  // RM0383 §6.3.3, offset 0x08
#define RCC_AHB1ENR (*(volatile uint32_t *)0x40023830)  // RM0383 §6.3.9, offset 0x30

#define RCC_CR_PLLON  (1 << 24)  // turn the main PLL on
#define RCC_CR_PLLRDY (1 << 25)  // read-only: PLL locked and ready

#define PLLCFGR_PLLM_POS   0            // PLLM[5:0]  — input divider
#define PLLCFGR_PLLN_POS   6            // PLLN[14:6] — VCO multiplier
#define PLLCFGR_PLLP_POS   16           // PLLP[17:16] — output divider (00 = /2)
#define PLLCFGR_PLLSRC     (1 << 22)    // 0 = HSI, 1 = HSE
#define PLLCFGR_PLLM_MASK  (0x3F  << PLLCFGR_PLLM_POS)
#define PLLCFGR_PLLN_MASK  (0x1FF << PLLCFGR_PLLN_POS)
#define PLLCFGR_PLLP_MASK  (0x3   << PLLCFGR_PLLP_POS)

#define CFGR_SW_MASK   (0x3 << 0)  // SW[1:0]  — requested SYSCLK source
#define CFGR_SW_HSI    (0x0 << 0)
#define CFGR_SW_PLL    (0x2 << 0)
#define CFGR_SWS_MASK  (0x3 << 2)  // SWS[3:2] — read-only: actual SYSCLK source
#define CFGR_SWS_HSI   (0x0 << 2)
#define CFGR_SWS_PLL   (0x2 << 2)

#define GPIOAEN (1 << 0)  // RCC_AHB1ENR bit 0 — clock to GPIO port A

#endif
