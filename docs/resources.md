# Resources

Hardware, reference documents and tools used in this project. The Wi-Fi
bridge is treated as a black box and isn't listed here.

## Hardware

### STM32 Nucleo-F411RE

The main board — STM32F411RE (Arm Cortex-M4), programmed
bare-metal (registers only, no HAL).

- Board page: <https://www.st.com/en/evaluation-tools/nucleo-f411re.html>
- Chip page: <https://www.st.com/en/microcontrollers-microprocessors/stm32f411re.html>

All the reference documents live on ST's website (**Documentation** tab on the
pages above):

| Document | What it's used for here |
|----------|-------------------------|
| **DS10314** — STM32F411xC/xE datasheet | Peripherals on the chip (e.g. 3 USARTs), pin table + alternate functions (USART1 on PA9/PA10), electrical limits |
| **RM0383** — STM32F411xC/xE reference manual | Every register: RCC, flash wait-states, GPIO, USART, ADC, memory map |
| **UM1724** — STM32 Nucleo-64 boards user manual | Board layout: which chip pin is on which header (D8 = PA9, D2 = PA10, A0 = PA0), LD2 on PA5, ST-LINK virtual COM port on USART2 |
| **PM0214** — Cortex-M4 programming manual | Core peripherals: SysTick |

### Analog microphone

Analog microphone module — electret capsule (GMI6027P-2C44DB) plus an
amplifier with a GAIN trimmer.

- Powered from the Nucleo's 3.3 V; analog output into A0 (PA0, ADC1 channel 0)
- Read as peak-to-peak loudness over a short window

## Tech stack

### Firmware

| Tool | Use |
|------|-----|
| C (bare-metal, no HAL) | All firmware, written register by register |
| [PlatformIO](https://platformio.org) | Build + upload only (ST platform pinned to an exact commit) |
| ST-LINK (on the Nucleo) + OpenOCD | Flashing and debugging |
| VS Code | Editor, PlatformIO debugger, serial monitor |

### Cloud (AWS)

| Service | Use |
|---------|-----|
| [AWS IoT Core](https://aws.amazon.com/iot-core/) | MQTT over TLS; device thing, X.509 certificate, device policy |
| IoT topic rule | Routes readings to the Lambda |
| [AWS Lambda](https://aws.amazon.com/lambda/) (Python) | Picks a clock level from each reading, publishes the command |
| [Amazon CloudWatch Logs](https://aws.amazon.com/cloudwatch/) | Lambda logs — one line per reading = the history |
| IAM | Lambda's permission to publish commands |
| [AWS CDK](https://aws.amazon.com/cdk/) (Python) | All of the above as code |

### Tooling

| Tool | Use |
|------|-----|
| Python + [uv](https://docs.astral.sh/uv/) | CDK app + Lambda dependencies |
| pytest | Unit tests for the infra and the Lambda |
| AWS CLI | Device certificate, IoT endpoint lookup |
| Git + Git Bash | Version control, running the CDK / AWS commands |
| [GitHub Actions](https://docs.github.com/actions) | CI: tests + firmware build when a pushed commit message contains `[ci:run-tests]` (`.github/workflows/ci.yml`) |
| [Claude Code](https://claude.com/claude-code) | AI pair-programmer used throughout |

## Related

- [README](../README.md)
- [protocol.md](protocol.md) — message formats between the board, the bridge and the cloud
- [how-to/local-testing.md](how-to/local-testing.md) — testing the firmware on the board alone
