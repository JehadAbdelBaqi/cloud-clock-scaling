# Firmware

Bare-metal C for the **STM32 Nucleo-F411RE** — every peripheral is set up
register by register, no HAL. PlatformIO is used only to build and upload.

What it does: reads the mic, sends readings to the Wi-Fi bridge over UART,
and switches the CPU clock between 16 MHz and 50 MHz when the cloud says so —
dropping back to 16 MHz on its own after a hold time.

## Layout

```
firmware/
├── platformio.ini        Build/upload config (ST platform pinned to a commit)
├── include/              Headers only — the code itself is in src/
│   ├── config/           Settings + config tables
│   │   ├── app_config.h    ALL app settings — what runs, timings, levels
│   │   ├── clock_config.h  Per-speed table: PLL M/N/P, flash wait-states
│   │   └── uart_config.h   Per-port table: USART, clock enable, pins
│   ├── drivers/          API headers — the functions each driver offers
│   │   ├── clock.h         clock_set(), clock_get_hz()
│   │   ├── uart.h          UART (any port): init, send, receive
│   │   └── mic.h           init, peak-to-peak loudness
│   ├── regs/             Register addresses + bit names, one file per peripheral
│   └── tests/            Test entry points
└── src/
    ├── main.c            Boot: 16 MHz, LED, SysTick, then hands over to a test
    ├── clock.c           Clock switching (HSI <-> PLL, wait-states)
    ├── uart.c            Polled UART driver
    ├── mic.c             ADC1 on PA0, peak-to-peak over a window
    └── tests/            Test programs (see below)
```

### `include/regs/` — hardware facts

Register addresses and bit positions only — no logic. Each one cites where it
comes from in ST's documents (see [../docs/resources.md](../docs/resources.md)).

| File | Peripheral |
|------|------------|
| `rcc.h` | Reset and clock control — PLL, clock source, peripheral clock enables |
| `flash.h` | Flash wait-states (must rise before the clock does) |
| `gpio.h` | GPIO port A — pin modes, alternate functions, pin numbers used |
| `systick.h` | SysTick timer (Cortex-M4 core) |
| `usart.h` | USART register layout (shared by all USARTs) + USART1/USART2 bases |
| `adc.h` | ADC1 |

### Drivers

| Module | Does | Notes |
|--------|------|-------|
| `clock.c` | `clock_set(CLOCK_16MHZ / CLOCK_50MHZ)` | Wait-states **up before** speeding up, **down after** slowing down. PLL settings come from `clock_config.h` |
| `uart.c` | `uart_init / puts / getc / flush / set_clock` per port | Polled. Baud divider is worked out from the CPU clock, so call `uart_set_clock()` after every clock switch |
| `mic.c` | `mic_poll()` → loudness each window | One ADC reading per call; loudness = max − min over `MIC_WINDOW_SAMPLES` |

UART ports (`config/uart_config.h`):

| Port | USART | Pins | Used for |
|------|-------|------|----------|
| `UART_PC` | USART2 | PA2 TX | Logging to the PC through the ST-LINK (USB) |
| `UART_BRIDGE` | USART1 | PA9 TX (D8) / PA10 RX (D2) | The Wi-Fi bridge |

### `src/tests/` — what runs

`main.c` boots the board, then hands over to the test picked in
`config/app_config.h`. Set **one** to `1`:

| Setting | Program | Needs |
|---------|---------|-------|
| `IS_LOCAL_TEST` | `clock_cycle_test.c` — flips 16 ↔ 50 MHz on every SysTick countdown | Board only |
| `IS_CLOUD_TEST` | `cloud_cycle_test.c` — mic → bridge → AWS → command → clock switch | Bridge running + stack deployed |

Shared helpers: `test_led.c` (LED blink that speeds up with the clock) and
`test_uart_log.c` (PC logging, `TEST_UART_LOG`).

## Settings — `include/config/app_config.h`

| Setting | Default | Meaning |
|---------|---------|---------|
| `BOOT_CLOCK` | `CLOCK_16MHZ` | Every boot starts here |
| `IS_LOCAL_TEST` / `IS_CLOUD_TEST` | `0` / `1` | Which program runs |
| `MIC_WINDOW_SAMPLES` | `256` | Readings per loudness window |
| `MIC_CLAP_LEVEL` | `100` | "Loud noise": a window with peak-to-peak ≥ this (ADC counts, ≈ 80 mV) is sent straight away — keep equal to the cloud's `medThreshold`. See the main README |
| `READING_INTERVAL_S` | `30` | Regular reading interval |
| `HOLD_S` | `15` | How long to stay at 50 MHz after a command |
| `BLINK_RELOAD` / `SYSTICK_DIV8` | `0xFFFFFF` / `1` | SysTick countdown length — also the timebase for the hold and reading interval |
| `TEST_SW_LED` / `TEST_SW_LED_LOOPS` | `1` / per test | LED blink mode + speed |
| `TEST_UART_LOG` | `1` | Log to the PC serial monitor |
| `UART_BAUD` | `115200` | Both UARTs — must match `monitor_speed` and the bridge |

## Build and upload

Open this folder in VS Code with PlatformIO → **Build**, **Upload** (over the
Nucleo's ST-LINK), plug icon for the serial monitor (COM4 — change
`monitor_port` in `platformio.ini` if yours differs).

Wiring and full test steps: [../docs/how-to/local-testing.md](../docs/how-to/local-testing.md).
Message formats: [../docs/protocol.md](../docs/protocol.md).
