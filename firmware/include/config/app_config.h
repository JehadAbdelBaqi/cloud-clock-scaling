#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// App settings — one place to change how the firmware behaves.
// Hardware facts (register addresses, bits) stay in include/regs/.

// Clock the board boots at — standard is 16 MHz (HSI), from clock.h
#define BOOT_CLOCK CLOCK_16MHZ

// SysTick reload (24-bit max) — never changed, so blink rate tracks the CPU clock
#define BLINK_RELOAD 0xFFFFFF

// SysTick clock: 0 = AHB (= SYSCLK), 1 = AHB/8 (8x slower blink)
#define SYSTICK_DIV8 1

// Pick at most one test. Both 0 = normal firmware.
// 1 = local clock-cycle test (flips 16 <-> 50 MHz on every SysTick underflow)
#define IS_LOCAL_TEST 0
// 1 = cloud-cycle test: mic readings -> Wi-Fi bridge -> AWS; commands back switch the clock
#define IS_CLOUD_TEST 1

// Mic (PA0 / A0). Loudness = peak-to-peak of the 12-bit ADC over one window.
// 256 samples ≈ a few ms — short enough to catch a clap's peak.
#define MIC_WINDOW_SAMPLES 256
// Clap level: a window this loud is sent straight away. Keep it equal to the
// cloud's medThreshold (infra/cdk.json) so a clap always gets a command back.
// Calibrate: watch the S lines' peaks for quiet vs a clap.
#define MIC_CLAP_LEVEL 100

// Regular (non-clap) reading interval. Every reading runs the cloud Lambda once.
// Counted in whole SysTick countdowns, so it lands a little over (up to ~8 s at 16 MHz).
#define READING_INTERVAL_S 30

// How long to stay at the raised clock after a command, before dropping back
// to 16 MHz. The board's decision — the cloud only says "speed up".
// Counted in whole SysTick countdowns (~2.7 s each at 50 MHz), so it lands a little over.
#define HOLD_S 15

// Tests only. 0 = LED toggles on each SysTick underflow
// 1 = LED driven by a software loop counter — blinks faster at 50 MHz
#define TEST_SW_LED 1

// Loop passes per LED toggle when TEST_SW_LED = 1 — tune by eye.
// The cloud test's loop is much slower (an ADC reading every pass), so it needs fewer.
#if IS_CLOUD_TEST
#define TEST_SW_LED_LOOPS 10000
#else
#define TEST_SW_LED_LOOPS 200000
#endif

// Tests only. 1 = log clock switches (and cloud-test lines) to the PC serial monitor
// (USART2 -> ST-LINK virtual COM port)
#define TEST_UART_LOG 1

// Baud for both UARTs. PC: must match monitor_speed in platformio.ini.
// Bridge: must match the Wi-Fi bridge's UART baud.
#define UART_BAUD 115200

#endif
