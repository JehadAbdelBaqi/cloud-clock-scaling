#ifndef CLOCK_H
#define CLOCK_H

typedef enum {
    CLOCK_16MHZ = 0,  // HSI, PLL off
    CLOCK_50MHZ = 1,  // main PLL from HSI
} clock_speed_t;

// Switch SYSCLK to the given speed. Safe to call from any current speed.
void clock_set(clock_speed_t speed);

#endif
