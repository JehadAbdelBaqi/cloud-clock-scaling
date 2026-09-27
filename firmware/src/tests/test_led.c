#include <stdint.h>

#include "tests/test_led.h"
#include "config/app_config.h"
#include "regs/gpio.h"

void test_led_loop(void) {
#if TEST_SW_LED
    // Loop runs faster on a faster CPU clock -> LED blinks faster at 50 MHz
    static uint32_t loops = 0;
    if (++loops >= TEST_SW_LED_LOOPS) {
        loops = 0;
        GPIOA_ODR ^= (1 << LED_PIN);
    }
#endif
}

void test_led_on_switch(void) {
#if !TEST_SW_LED
    // SysTick reload is never touched, so each LED phase shows the clock it ran on:
    // ~1.05 s at 16 MHz, ~0.34 s at 50 MHz (x8 with SYSTICK_DIV8 = 1)
    GPIOA_ODR ^= (1 << LED_PIN);
#endif
}
