#include <stdint.h>

#include "app_config.h"
#include "clock.h"
#include "regs/gpio.h"
#include "regs/rcc.h"
#include "regs/systick.h"
#include "tests/clock_cycle_test.h"

int main(void) {
    clock_set(BOOT_CLOCK);                           // standard: every boot starts at 16 MHz

    RCC_AHB1ENR |= GPIOAEN;                          // enable GPIOA clock
    GPIOA_MODER |= (1 << (LED_PIN * 2));             // LED pin mode = 01 (general purpose output)
    STK_LOAD = BLINK_RELOAD;                          // set reload value for SysTick
    STK_CTRL = STK_CTRL_ENABLE                        // enable SysTick; CLKSOURCE 1 = AHB, 0 = AHB/8
             | (SYSTICK_DIV8 ? 0 : STK_CTRL_CLKSOURCE);

#if IS_LOCAL_TEST
    clock_cycle_test_run();                          // never returns
#endif

    while (1) {}
}
