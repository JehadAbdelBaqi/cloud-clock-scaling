# Cloud-Controlled Clock Scaling — STM32 + AWS IoT

A clap near an STM32 board changes the board's CPU clock speed — decided in
the cloud. Sound readings go up to AWS IoT Core, a Lambda picks a clock
level, and the command comes back down to the board, which reconfigures its
PLL at runtime. When it goes quiet, the board drops itself back to low speed.

> **Status: in progress.** See [Roadmap](#roadmap) for what works today.

## Why

**Dynamic frequency scaling** — run the CPU slow when idle to save power,
speed it up only when there's work to do. This project drives that decision
from the cloud, end to end:

- **Embedded:** bare-metal STM32 (register-level, no HAL) — PLL
  reconfiguration, flash wait-states, ADC, UART, timers
- **IoT:** device ↔ cloud round trip over MQTT, X.509 device identity
- **DevOps:** all AWS resources in CDK, unit-tested *(GitHub Actions
  pipeline + firmware build in CI: planned)*

## Architecture

```
  CLAP                                               AWS
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
| HIGH | 100 MHz | PLL (chip max) — *not in the firmware yet* |

**Edge vs cloud split:** the cloud decides when to speed up; the board is
responsible for dropping back to low speed on its own after a hold time — so
it always returns to its low-power state, even if the network is down.

## Hardware

| Part | Role |
|------|------|
| ST **Nucleo-F411RE** (STM32F411RE, Cortex-M4F) | The device |
| Analog microphone module | Sound input — A0 (PA0), ADC1 |
| **Wi-Fi bridge** (ESP32-S3 board) | The Nucleo has no Wi-Fi. A separate UART-to-MQTT bridge project, treated as a black box: the Nucleo sends and receives plain text lines over UART (USART1, D8/D2), the bridge passes them to and from AWS IoT Core |

## Documentation

| Doc | What's in it |
|-----|--------------|
| [firmware/README.md](firmware/README.md) | STM32 firmware — layout, drivers, settings, build/upload |
| [lambda/README.md](lambda/README.md) | The cloud's decision logic |
| [infra/README.md](infra/README.md) | AWS resources (CDK), settings, deploy |
| [docs/protocol.md](docs/protocol.md) | Message formats: board ↔ bridge ↔ cloud |
| [docs/decisions.md](docs/decisions.md) | Design decisions and why |
| [docs/resources.md](docs/resources.md) | Hardware, ST datasheets/manuals, tech stack |
| [docs/how-to/local-testing.md](docs/how-to/local-testing.md) | Running and checking the test programs |

## Repo layout

```
firmware/     STM32 firmware (PlatformIO, bare-metal)
lambda/       Clock-level decision logic
infra/        AWS CDK app (IoT Core, Lambda)
docs/         Message protocol, design decisions, resources, how-to guides
```

Each part has its own README: [firmware](firmware/README.md) ·
[lambda](lambda/README.md) · [infra](infra/README.md). Why things are built
the way they are: [docs/decisions.md](docs/decisions.md).

## Roadmap

- [x] Clock switcher on the board (16 ↔ 50 MHz)
- [ ] 100 MHz level
- [x] AWS infra in CDK (IoT thing/policy/rule, Lambda, log group)
- [x] Wi-Fi bridge publishing readings to IoT Core
- [x] Full round trip: Lambda → command → board switches clock
- [x] Hold timer on the board → back to low speed
- [ ] Mic → ADC → clap detection *(built, being tested + calibrated)*
- [ ] GitHub Actions (tests, `cdk synth`, firmware build, deploy)
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
uv sync          # creates .venv and installs everything from uv.lock
uv run pytest    # infra and lambda tests
```

### 2. Deploy to AWS

```bash
export AWS_PROFILE=clockscale        # your SSO profile name
aws sso login

aws iot describe-endpoint --endpoint-type iot:Data-ATS
# put the address into infra/cdk.json -> "iotEndpoint"

# device cert is created by the UART-MQTT bridge project (the key lives there);
# put its ARN into infra/cdk.json -> "certificateArn"

cd infra
cdk bootstrap                        # once per account/region
cdk deploy ClockScaleStack          # from Git Bash: add --require-approval never
```

The Wi-Fi bridge connects to that same `iotEndpoint` address.

Watch it arrive: AWS console → IoT Core → **MQTT test client** → subscribe
to `clockscale/#`.

### Settings (`infra/cdk.json` → `context`)

| Key | Default | Meaning |
|-----|---------|---------|
| `iotEndpoint` | — | This account's IoT data endpoint (`aws iot describe-endpoint --endpoint-type iot:Data-ATS`) |
| `deviceId` | `nucleo-01` | IoT thing name + MQTT client ID |
| `certificateArn` | — | Device cert ARN — the cert itself is made and kept outside this repo |
| `medThreshold` / `highThreshold` | 100 / 300 | Peak level for MED / HIGH *(to calibrate)* |
| `maxLevel` | 1 | Highest level the cloud will send (keep at 1 until the firmware has a 100 MHz setting) |

### 3. Firmware

Open `firmware/` in VS Code with PlatformIO → **Build** / **Upload** (over the
Nucleo's ST-LINK). What runs is picked in `firmware/include/config/app_config.h`:

| Setting | Runs |
|---------|------|
| `IS_LOCAL_TEST = 1` | Clock-cycle test — board alone, no cloud |
| `IS_CLOUD_TEST = 1` | Mic → bridge → AWS → command → clock switch |

Only one at a time. Wiring and pins: [docs/resources.md](docs/resources.md) and
[docs/protocol.md](docs/protocol.md).

See [docs/protocol.md](docs/protocol.md) for the message formats,
[docs/resources.md](docs/resources.md) for the hardware, datasheets and tech stack,
and [docs/how-to/local-testing.md](docs/how-to/local-testing.md) for testing on the board alone.
