#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

typedef enum {
    CLOCK_16MHZ = 0,  // HSI, PLL off
    CLOCK_50MHZ = 1,  // main PLL from HSI
} clock_speed_t;

// Switch SYSCLK to the given speed. Safe to call from any current speed.
void clock_set(clock_speed_t speed);

// Current SYSCLK in Hz.
uint32_t clock_get_hz(void);

#endif
