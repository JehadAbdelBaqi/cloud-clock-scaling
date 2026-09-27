#ifndef CLOCK_CONFIG_H
#define CLOCK_CONFIG_H

#include <stdint.h>

#include "clock.h"

// Settings for each clock speed — used by clock_set() in clock.c.
// PLL maths: HSI 16 MHz / M * N / P.
typedef struct {
    uint32_t hz;       // resulting SYSCLK
    uint8_t  use_pll;
    uint8_t  pll_m;
    uint16_t pll_n;
    uint8_t  pll_p;    // PLLP field value: 0 = /2, 1 = /4, 2 = /6, 3 = /8
    uint8_t  latency;  // flash wait-states (RM0383 Table 5, 2.7–3.6 V)
} clock_cfg_t;

static const clock_cfg_t clock_cfg[] = {
    [CLOCK_16MHZ] = { .hz = 16000000, .use_pll = 0,                                      .latency = 0 },
    [CLOCK_50MHZ] = { .hz = 50000000, .use_pll = 1, .pll_m = 8, .pll_n = 50, .pll_p = 0, .latency = 1 },  // 16/8*50/2
};

#endif
