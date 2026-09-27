# Local Testing

How to check the firmware's clock switching. Each test type is a build-time
switch in `firmware/include/config/app_config.h` — set **one** of them to `1`.

## How it works

- Every boot starts at `BOOT_CLOCK` (16 MHz) — always, test or not.
- `IS_LOCAL_TEST = 1` or `IS_CLOUD_TEST = 1` → `main()` hands over to that
  test after setup. The test never returns.
- A test set to `0` is removed at build time (`#if`); it isn't in the firmware
  at all.
- Change a setting → rebuild + upload. Nothing is read at run time.

## Test types

| Test | What triggers a switch | Code |
|------|------------------------|------|
| Clock cycle (`IS_LOCAL_TEST`) — board alone | Every SysTick countdown to 0 | `firmware/src/tests/clock_cycle_test.c` |
| Cloud cycle (`IS_CLOUD_TEST`) — needs the Wi-Fi bridge + AWS | A clap → command from the cloud | `firmware/src/tests/cloud_cycle_test.c` |

## Clock cycle test

Flips 16 MHz ↔ 50 MHz every time SysTick counts down to 0. LD2 shows the clock
one of two ways, picked by `TEST_SW_LED`:

| `TEST_SW_LED` | LED driven by | What you see |
|---------------|---------------|--------------|
| `0` | SysTick — toggles with each switch | Long phase = 16 MHz, short phase = 50 MHz |
| `1` | Software loop counter | Slow blink = 16 MHz, fast blink = 50 MHz |

**Software counter:** the test loop counts passes and toggles LD2 every
`TEST_SW_LED_LOOPS`. A faster CPU clock runs the loop faster, so the blink speeds
up. Not exactly 50/16 = 3.1× — flash wait-states at 50 MHz slow each pass a bit.

**Why SysTick phases show the clock:** SysTick counts CPU clock ticks (or CPU clock ÷ 8)
and its reload is never changed. So the same count takes longer at 16 MHz than
at 50 MHz — a long LED phase = 16 MHz, a short one = 50 MHz.

### Settings (`app_config.h`)

| Setting | Value | Why |
|---------|-------|-----|
| `IS_LOCAL_TEST` | `1` | Runs the test (`IS_CLOUD_TEST` = `0`) |
| `BLINK_RELOAD` | `0xFFFFFF` | SysTick max (24-bit) — longest possible count |
| `SYSTICK_DIV8` | `1` | SysTick on CPU clock ÷ 8 — slow enough to see both phases |
| `TEST_SW_LED` | `0` / `1` | LED from SysTick / from the software counter |
| `TEST_SW_LED_LOOPS` | `200000` | Loop passes per toggle — tune by eye (the cloud test uses `10000`: its loop is slower) |
| `TEST_UART_LOG` | `1` | Log each switch to the PC serial monitor (USART2, `UART_PC`) |
| `UART_BAUD` | `115200` | Must match `monitor_speed` in `platformio.ini` |
| `BOOT_CLOCK` | `CLOCK_16MHZ` | Test starts from the standard boot speed |

### Expected (SysTick phase lengths)

| `SYSTICK_DIV8` | 16 MHz phase | 50 MHz phase |
|----------------|--------------|--------------|
| `0` | ~1.05 s | ~0.34 s — hard to see |
| `1` | ~8.4 s | ~2.7 s |

Maths: `(BLINK_RELOAD + 1) / SysTick clock` — e.g. 16,777,216 / 2 MHz ≈ 8.4 s.

### Run it

1. Set the settings above in `app_config.h`.
2. PlatformIO → **Build**, then **Upload**.
3. Watch LD2 (the green user LED): long/short phases (`TEST_SW_LED = 0`) or
   slow/fast blinking (`TEST_SW_LED = 1`).

### Log to the PC (optional)

With `TEST_UART_LOG = 1` the test prints each switch over USART2 (PA2), which the
Nucleo's ST-LINK passes to the PC as a virtual COM port over the same USB cable.
**Local testing only** — the UART code is built in only inside the test, never
in the normal firmware.

1. Build + upload.
2. PlatformIO toolbar → **plug icon** (Serial Monitor). Opens COM4 at 115200
   (`monitor_port` / `monitor_speed` in `platformio.ini`) — change `monitor_port`
   if the Nucleo shows up on a different port (`pio device list`).
3. Press **RESET** on the board to see it from the start:

```
test start: 16 MHz
50 MHz
16 MHz
50 MHz
...
```

**Why the text stays readable across switches:** the baud divider (`BRR`) is
worked out from the CPU clock. On every switch the test waits for the last byte
to finish (`uart_flush`), changes the clock, then recomputes `BRR`
(`uart_set_clock`). Skip that and every line after a switch is garbage.

## Cloud cycle test

The full loop: mic → Nucleo → Wi-Fi bridge → AWS → command back → clock switch.
Needs the bridge running and wired (see [protocol.md](../protocol.md)) and the
stack deployed.

| Setting | Value | Why |
|---------|-------|-----|
| `IS_CLOUD_TEST` | `1` | Runs the test (`IS_LOCAL_TEST` = `0`) |
| `MIC_CLAP_LEVEL` | `100` | A window this loud is sent straight away — keep equal to the cloud's `medThreshold` |
| `READING_INTERVAL_S` | `30` | Regular reading (loudest level since the last one) |
| `HOLD_S` | `15` | How long to stay at 50 MHz after a command |

With `TEST_UART_LOG = 1` the PC monitor shows every line sent (`->`) and
received (`<-`). A clap should give:

```
-> S,4,420
<- C,4,1
50 MHz
-> A,4,50
... ~15 s later
16 MHz
-> A,4,16
```

LD2 blinks fast while at 50 MHz. If quiet readings come close to
`MIC_CLAP_LEVEL`, raise it together with `medThreshold`, or turn the mic's GAIN
trimmer down.

**Monitor goes quiet after RESET** (board still blinking) → close it (Ctrl+C)
and reopen it. Cause not confirmed yet.

### Check it in the debugger (optional)

1. PlatformIO → **Start Debugging** (F5). It pauses at the start of `main()` —
   press F5 again to continue.
2. **Watch** panel → **+** → `clock_speed_mhz`.
3. Breakpoint just after `clock_speed_mhz = ...` in the test. Each F5 stops at
   the next switch — the value alternates 16 / 50.
4. **Shift+F5** to stop debugging.

`clock_speed_mhz` shows the speed the code asked for, not a register read.
`clock_set()` only returns once the hardware confirms the switch (SWS bits), so
it's a good sign but not proof.

## Related

- [protocol.md](../protocol.md) — clock levels and message formats
- [resources.md](../resources.md) — hardware, reference documents, tech stack
- [README](../../README.md)
