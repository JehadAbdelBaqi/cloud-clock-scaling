# Cloud-Controlled Clock Scaling — STM32 + AWS IoT

[![CI](https://github.com/JehadAbdelBaqi/cloud-clock-scaling/actions/workflows/ci.yml/badge.svg)](https://github.com/JehadAbdelBaqi/cloud-clock-scaling/actions/workflows/ci.yml)

Cloud-driven **dynamic frequency scaling** for a bare-metal STM32F411
(Cortex-M4). The board samples sound through its ADC and reports the level
over MQTT to AWS IoT Core; a Lambda decides the clock level and publishes a
command back; the firmware switches SYSCLK at runtime between the internal
16 MHz oscillator and the PLL (50 MHz) — adjusting flash wait-states and
UART baud rates on every switch — then returns to 16 MHz on its own after a
hold time.

> **Status:** working end to end on real hardware. Next: measuring the
> round-trip latency on an oscilloscope — see [Roadmap](#roadmap).

## How it works

1. **Listen** — the Nucleo reads an analog mic on its ADC and measures
   loudness as peak-to-peak over short windows.
2. **Report** — any loud noise above a threshold is sent straight away
   (plus a regular reading every 30 s) as a text line over UART to a Wi-Fi
   bridge, which publishes it to AWS IoT Core over MQTT/TLS.
3. **Decide** — an IoT rule invokes a Lambda, which maps the loudness to a
   clock level and publishes a command to the device's topic.
4. **Switch** — the command comes back through the bridge; the firmware
   raises flash wait-states, moves SYSCLK onto the PLL at 50 MHz, recomputes
   its UART baud dividers and acknowledges.
5. **Fall back** — after the hold time the board switches back to 16 MHz
   by itself, whether or not the cloud is reachable.

### What counts as a "loud noise"

The mic module outputs a voltage that swings up and down with the sound
wave, around a steady middle level. The ADC reads it as a 12-bit number:
0–4095 for 0–3.3 V (≈ 0.8 mV per step).

- **Loudness** = the **peak-to-peak** of those readings over one window of
  256 samples (a few ms): highest reading − lowest reading. Silence gives a
  small number; a louder sound swings further and gives a bigger one.
- **Loud noise** = a window with loudness **≥ 100** (≈ 80 mV of swing) —
  `MIC_CLAP_LEVEL` on the board, the same value as `medThreshold` in the
  cloud, so every loud noise the board sends gets a 50 MHz command back.
- The number depends on the mic module's **GAIN** trimmer and how far the
  sound is from the mic — 100 is a starting point to calibrate against
  real quiet vs loud readings.

## Why

**Dynamic frequency scaling** — run the CPU slow when idle to save power,
speed it up only when there's work to do. This project drives that decision
from the cloud, end to end:

- **Embedded:** bare-metal STM32 (register-level, no HAL) — PLL
  reconfiguration, flash wait-states, ADC, UART, SysTick
- **IoT:** device ↔ cloud round trip over MQTT, X.509 device identity
- **DevOps:** all AWS resources in CDK, unit-tested; GitHub Actions runs the
  tests and builds the firmware on pushes marked `[ci:run-tests]` (deploys are run by hand)

## Architecture

```
  LOUD NOISE                                         AWS
   │                                       ┌───────────────────────────────┐
   ▼                                       │                               │
 [Analog mic]                              │  IoT Core ── Rule ──► Lambda  │
   │ ADC                                   │   ▲                     │     │
   ▼                                       │   │ readings            │     │
 [STM32 Nucleo-F411RE]  UART   [Wi-Fi  ]   │   │                     ▼     │
   sample → peak ─────────────►[bridge ]─MQTT──┘          picks clock level│
   │                  ◄────────[       ]◄MQTT─── IoT Core ◄── command ─┘   │
   ▼                  command              │                               │
 reconfigure PLL                           │  CloudWatch Logs (history)    │
 SYSCLK 16 / 50 MHz                        └───────────────────────────────┘
 hold timer → back to 16 MHz when quiet
```

| Clock level | SYSCLK | Source |
|-------------|--------|--------|
| LOW | 16 MHz | HSI (internal oscillator) |
| MED | 50 MHz | PLL |

**Edge vs cloud split:** the cloud decides when to speed up; the board is
responsible for dropping back to low speed on its own after a hold time — so
it always returns to its low-power state, even if the network is down.

## Hardware

| Part | Role |
|------|------|
| ST **Nucleo-F411RE** (STM32F411RE, Cortex-M4) | The device |
| Analog microphone module | Sound input — A0 (PA0), ADC1 |
| **Wi-Fi bridge** (ESP32-S3 board) | The Nucleo has no Wi-Fi. A separate UART-to-MQTT bridge project, treated as a black box: the Nucleo sends and receives plain text lines over UART (USART1, D8/D2), the bridge passes them to and from AWS IoT Core |

### Wiring

| From | To (Nucleo header) | Signal |
|------|--------------------|--------|
| Mic power | 3V3 | 3.3 V — not 5V, the ADC pin takes up to 3.3 V |
| Mic GND | GND | Ground |
| Mic analog out | A0 (PA0) | ADC1 channel 0 |
| Bridge RX | D8 (PA9) | Nucleo USART1 TX |
| Bridge TX | D2 (PA10) | Nucleo USART1 RX |
| Bridge GND | GND | Common ground |

UART: 115200 baud, 8N1, 3.3 V logic on both sides. The Nucleo's USB (ST-LINK)
powers it, flashes it, and carries the PC log (USART2).

## Documentation

| Doc | What's in it |
|-----|--------------|
| [firmware/README.md](firmware/README.md) | STM32 firmware — layout, drivers, settings, build/upload |
| [infra/cdk/README.md](infra/cdk/README.md) | AWS resources (CDK), settings, deploy |
| [infra/decide-clock-lambda/README.md](infra/decide-clock-lambda/README.md) | The Lambda: the cloud's decision logic |
| [docs/protocol.md](docs/protocol.md) | Message formats: board ↔ bridge ↔ cloud |
| [docs/decisions.md](docs/decisions.md) | Design decisions and why |
| [docs/resources.md](docs/resources.md) | Hardware, ST datasheets/manuals, tech stack |
| [docs/how-to/local-testing.md](docs/how-to/local-testing.md) | Running and checking the test programs |

## Repo layout

```
firmware/     STM32 firmware (PlatformIO, bare-metal)
infra/
  cdk/                  AWS CDK app (IoT Core, Lambda, log group)
  decide-clock-lambda/  Lambda: clock-level decision logic
docs/         Message protocol, design decisions, resources, how-to guides
```

## Roadmap

- [x] Clock switcher on the board (16 ↔ 50 MHz)
- [x] AWS infra in CDK (IoT thing/policy/rule, Lambda, log group)
- [x] Wi-Fi bridge publishing readings to IoT Core
- [x] Full round trip: Lambda → command → board switches clock
- [x] Hold timer on the board → back to low speed
- [x] Mic → ADC → loud-noise detection
- [x] GitHub Actions: tests + firmware build on pushes marked `[ci:run-tests]` (no deploy)
- [ ] End-to-end latency measured on an oscilloscope

**Nice to have:** a small LCD on the board showing the current clock speed.

## Getting started

**Prerequisites:** [uv](https://docs.astral.sh/uv/), Node (for the CDK
CLI), AWS CLI v2 with an SSO profile (`aws configure sso`),
`npm install -g aws-cdk`.

Python dependencies live in one `pyproject.toml` at the repo root, managed by
uv — one venv for infra and Lambda. All commands below run from the
**repo root** unless they say otherwise.

### 1. Install + run the tests (no AWS needed)

```bash
uv sync          # creates .venv, installs from uv.lock (PlatformIO excluded)
uv run pytest    # CDK stack + Lambda tests
```

### 2. Deploy to AWS

```bash
export AWS_PROFILE=clockscale        # your SSO profile name
aws sso login

aws iot describe-endpoint --endpoint-type iot:Data-ATS
# put the address into infra/cdk/cdk.json -> "iotEndpoint"

# device cert is created by the UART-MQTT bridge project (the key lives there);
# put its ARN into infra/cdk/cdk.json -> "certificateArn"

cd infra/cdk
cdk bootstrap                        # once per account/region
cdk deploy ClockScaleStack          # from Git Bash: add --require-approval never
```

The Wi-Fi bridge connects to that same `iotEndpoint` address.

Watch it arrive: AWS console → IoT Core → **MQTT test client** → subscribe
to `clockscale/#`.

### Settings (`infra/cdk/cdk.json` → `context`)

| Key | Default | Meaning |
|-----|---------|---------|
| `iotEndpoint` | — | This account's IoT data endpoint (`aws iot describe-endpoint --endpoint-type iot:Data-ATS`) |
| `deviceId` | `nucleo-01` | IoT thing name + MQTT client ID |
| `certificateArn` | — | Device cert ARN — the cert itself is made and kept outside this repo |
| `medThreshold` | 100 | Loudness that gets a 50 MHz command *(to calibrate)* |
| `maxLevel` | 1 | Highest level the cloud sends (1 = 50 MHz) |

### 3. Firmware

Open `firmware/` in VS Code with PlatformIO → **Build** / **Upload** (over the
Nucleo's ST-LINK). What runs is picked in `firmware/include/config/app_config.h`:

| Setting | Runs |
|---------|------|
| `IS_LOCAL_TEST = 1` | Clock-cycle test — board alone, no cloud |
| `IS_CLOUD_TEST = 1` | Mic → bridge → AWS → command → clock switch |

Only one at a time. Wiring: [above](#wiring). Test steps and expected output:
[docs/how-to/local-testing.md](docs/how-to/local-testing.md).
