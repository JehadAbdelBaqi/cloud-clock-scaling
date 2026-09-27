#include "tests/clock_cycle_test.h"

#include "app_config.h"
#include "clock.h"
#include "regs/gpio.h"
#include "regs/systick.h"

// Current SYSCLK in MHz — add to the debugger Watch panel to see each switch.
// volatile: the compiler must really write it every time, never optimise it out.
static volatile uint32_t clock_speed_mhz = 16;

void clock_cycle_test_run(void) {
    clock_speed_t speed = BOOT_CLOCK;                 // main() already put us here at boot
#if TEST_SW_LED
    uint32_t loops = 0;
#endif

    while (1) {
#if TEST_SW_LED
        // Loop runs faster on a faster CPU clock -> LED blinks faster at 50 MHz
        if (++loops >= TEST_SW_LED_LOOPS) {
            loops = 0;
            GPIOA_ODR ^= (1 << LED_PIN);
        }
#endif

        if (STK_CTRL & STK_CTRL_COUNTFLAG) {          // SysTick counted down to 0
#if !TEST_SW_LED
            // reload is never touched, so each LED phase shows the clock it ran on:
            // ~1.05 s at 16 MHz, ~0.34 s at 50 MHz (x8 with SYSTICK_DIV8 = 1)
            GPIOA_ODR ^= (1 << LED_PIN);
#endif
            speed = (speed == CLOCK_16MHZ) ? CLOCK_50MHZ : CLOCK_16MHZ;
            clock_set(speed);
            clock_speed_mhz = (speed == CLOCK_50MHZ) ? 50 : 16;
        }
    }
}
