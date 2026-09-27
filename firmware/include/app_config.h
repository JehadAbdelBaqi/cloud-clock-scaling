#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// App settings — one place to change how the firmware behaves.
// Hardware facts (register addresses, bits) stay in include/regs/.

// Clock the board boots at — standard is 16 MHz (HSI), from clock.h
#define BOOT_CLOCK CLOCK_16MHZ

// Status LED: PA5 = LD2 (port A pin, from regs/gpio.h)
#define LED_PIN PIN5

// SysTick reload (24-bit max) — never changed, so blink rate tracks the CPU clock
#define BLINK_RELOAD 0xFFFFFF

// SysTick clock: 0 = AHB (= SYSCLK), 1 = AHB/8 (8x slower blink)
#define SYSTICK_DIV8 1

// 1 = run the local clock-cycle test (flips 16 <-> 50 MHz on every SysTick underflow)
// 0 = normal firmware
#define IS_LOCAL_TEST 1

// Local test only. 0 = LED toggles on each SysTick underflow (with the switch)
// 1 = LED driven by a software loop counter — blinks faster at 50 MHz
#define TEST_SW_LED 1

// Loop passes per LED toggle when TEST_SW_LED = 1 — tune by eye
#define TEST_SW_LED_LOOPS 200000

#endif
