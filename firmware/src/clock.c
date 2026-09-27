#include <stdint.h>

#include "drivers/clock.h"
#include "config/clock_config.h"
#include "regs/flash.h"
#include "regs/rcc.h"

static clock_speed_t current = CLOCK_16MHZ;  // reset default: HSI

static void set_latency(uint8_t ws) {
    FLASH_ACR = (FLASH_ACR & ~FLASH_ACR_LATENCY_MASK) | ws;
    while ((FLASH_ACR & FLASH_ACR_LATENCY_MASK) != ws) {}  // confirm before moving on
}

static void switch_sysclk(uint32_t sw, uint32_t sws) {
    RCC_CFGR = (RCC_CFGR & ~CFGR_SW_MASK) | sw;
    while ((RCC_CFGR & CFGR_SWS_MASK) != sws) {}           // switch isn't instant
}

void clock_set(clock_speed_t speed) {
    const clock_cfg_t *cfg = &clock_cfg[speed];
    uint8_t current_ws = FLASH_ACR & FLASH_ACR_LATENCY_MASK;

    // Going faster: wait-states up FIRST, so flash keeps up with the new clock.
    if (cfg->latency > current_ws) {
        set_latency(cfg->latency);
    }

    // PLL settings are only writable while it's off — so get off the PLL first.
    switch_sysclk(CFGR_SW_HSI, CFGR_SWS_HSI);
    RCC_CR &= ~RCC_CR_PLLON;
    while (RCC_CR & RCC_CR_PLLRDY) {}

    if (cfg->use_pll) {
        RCC_PLLCFGR = (RCC_PLLCFGR
                       & ~(PLLCFGR_PLLM_MASK | PLLCFGR_PLLN_MASK | PLLCFGR_PLLP_MASK | PLLCFGR_PLLSRC))
                      | ((uint32_t)cfg->pll_m << PLLCFGR_PLLM_POS)
                      | ((uint32_t)cfg->pll_n << PLLCFGR_PLLN_POS)
                      | ((uint32_t)cfg->pll_p << PLLCFGR_PLLP_POS);  // PLLSRC 0 = HSI

        RCC_CR |= RCC_CR_PLLON;
        while (!(RCC_CR & RCC_CR_PLLRDY)) {}                          // wait for lock
        switch_sysclk(CFGR_SW_PLL, CFGR_SWS_PLL);
    }

    // Going slower: wait-states down LAST, once the clock is already slow.
    if (cfg->latency < current_ws) {
        set_latency(cfg->latency);
    }

    current = speed;
}

uint32_t clock_get_hz(void) {
    return clock_cfg[current].hz;
}
