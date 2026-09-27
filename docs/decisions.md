# Design Decisions

The main choices behind the build, and why. Newest last.

| # | Decision | Why | Instead of |
|---|----------|-----|------------|
| 1 | **Bare-metal firmware** — registers only, no HAL | Shows (and teaches) what the chip actually does: PLL, wait-states, USART, ADC by hand. PlatformIO only builds and uploads | STM32 HAL / CubeMX-generated code |
| 2 | **ST platform pinned to an exact commit** | A moved tag or new release can't change what builds | Latest platform version |
| 3 | **Every boot starts at 16 MHz** | One known, low-power starting point, whatever state the board was in | Remembering the last speed |
| 4 | **Wait-states up before speeding up, down after slowing down** | Flash must always keep up with the CPU; the wrong order can read garbage from flash | One fixed order |
| 5 | **All AWS resources in CDK**, in Python | Repeatable, reviewable, testable with unit tests | Clicking in the console |
| 6 | **No database** — the Lambda's log group is the history | One JSON log line per reading is enough, searchable in Logs Insights; nothing to run or pay for | DynamoDB table |
| 7 | **IoT endpoint as a setting** in `cdk.json` | It never changes per account/region; a custom resource to look it up was extra moving parts and its own log group | CDK custom resource looking it up at deploy |
| 8 | **Device certificate made outside the stack**, only its ARN in this repo | CloudFormation can't return a private key; the key lives only where the device is. An ARN isn't secret | Certificate created by CDK |
| 9 | **Wi-Fi bridge as a separate black box** | The Nucleo has no Wi-Fi. This project only depends on the bridge's interface (UART lines + wiring), like a bought Wi-Fi module | Wi-Fi code inside this project |
| 10 | **Custom bridge instead of ESP-AT** | ESP-AT (Espressif's ready-made AT-command firmware) doesn't support the ESP32-S3 — no build, no plans (esp-at issues #607, #956) | ESP-AT on the ESP32-S3 |
| 11 | **Plain text lines over UART** (`S,…` / `C,…` / `A,…`) | Readable in any serial monitor, trivial to parse by hand on the STM32 | JSON or binary on the UART |
| 12 | **USART1 for the bridge** (PA9/PA10) | USART2 is wired to the ST-LINK — kept for PC logging | Sharing one UART |
| 13 | **One UART driver, per-port table** | Same registers on every USART; adding a port = one table row | A copy of the driver per port |
| 14 | **Polled I/O, no interrupts (yet)** | Simplest to reason about; enough for short, rare command lines. The USART holds one byte, so bytes can be lost if the loop stalls — interrupts are the fix if that shows up | Interrupt-driven UART/ADC from the start |
| 15 | **Recompute the UART baud after every clock switch** | The baud divider is worked out from the CPU clock — same divider on a new clock = wrong baud, garbage text | Fixed divider |
| 16 | **Tests as build-time flags** (`IS_LOCAL_TEST`, `IS_CLOUD_TEST`) in one config file | A test that's off isn't in the firmware at all; every setting in one place | Run-time switches |
| 17 | **Mic on the Nucleo, not the bridge** | Sensors belong on the main controller; the Wi-Fi chip only does the network. Keeps the ADC work on the STM32 | Plugging the mic into the bridge (easier wiring) |
| 18 | **Loudness = peak-to-peak over a short window** | Size of the sound wave; a short window catches a clap's peak | Single samples / averages |
| 19 | **Readings every 30 s + straight away on a clap** | Every reading runs the Lambda; constant readings were wasted runs and log lines. Claps are the only readings that change anything | A reading every few seconds |
| 20 | **The cloud decides *speed up*; the board owns *how long* and *slowing down*** | The board still returns to low power if the network or cloud is down; the hold time is the device's behaviour, not the cloud's | Cloud sending the hold time and a "slow down" command |

## Related

- [README](../README.md)
- [protocol.md](protocol.md) — the message formats these decisions produced
- [resources.md](resources.md) — hardware, documents, tech stack
