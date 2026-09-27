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
- **DevOps:** all AWS resources in CDK, deployed through GitHub Actions,
  with the firmware built in CI too

## Architecture

```
  CLAP                                         AWS
   │                                 ┌───────────────────────────────┐
   ▼                                 │                               │
 [Analog mic]                        │  IoT Core ── Rule ──► Lambda  │
   │ ADC                             │   ▲                     │     │
   ▼                                 │   │ readings            │     │
 [STM32 Nucleo-F411RE]   UART        │   │                     ▼     │
   sample → peak ──────► [ESP32-S3] ─MQTT┘          picks clock level│
   │                          ▲      │                         │     │
   │                          └─MQTT─┼── IoT Core ◄── command ─┘     │
   ▼                                 │                               │
 reconfigure PLL                     │  CloudWatch Logs (history)    │
 SYSCLK 16 / 50 / 100 MHz            └───────────────────────────────┘
 hold timer → back to 16 MHz when quiet
```

| Clock level | SYSCLK | Source |
|-------------|--------|--------|
| LOW | 16 MHz | HSI (internal oscillator) |
| MED | 50 MHz | PLL |
| HIGH | 100 MHz | PLL (chip max) |

**Edge vs cloud split:** the cloud decides when to speed up; the board is
responsible for dropping back to low speed on its own after a hold time — so
it always returns to its low-power state, even if the network is down.

## Hardware

| Part | Role |
|------|------|
| ST **Nucleo-F411RE** (STM32F411RE, Cortex-M4F) | The device |
| Analog microphone module | Sound input (ADC) |
| **ESP32-S3** (Axiometa Genesis Mini) running ESP-AT | Wi-Fi co-processor — the Nucleo drives it with AT commands over UART (the Nucleo has no Wi-Fi) |

## Repo layout

```
firmware/     STM32 firmware (PlatformIO, bare-metal)
lambda/       Clock-level decision logic
infra/        AWS CDK app (IoT Core, Lambda)
docs/         Diagrams, wiring, scope captures
```

## Roadmap

- [ ] Clock switcher on the board (16 ↔ 50 MHz, then 100 MHz)
- [ ] Mic → ADC → clap detection
- [ ] AWS infra in CDK (IoT thing/policy/rule, Lambda, log group)
- [ ] ESP32-S3 (ESP-AT) publishing readings to IoT Core
- [ ] Full round trip: Lambda → command → board switches clock
- [ ] Hold-timer decay back to low speed
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
cdk deploy ClockScaleStack
```

The ESP32-S3 connects to that same `iotEndpoint` address.

Watch it arrive: AWS console → IoT Core → **MQTT test client** → subscribe
to `clockscale/#`.

### Settings (`infra/cdk.json` → `context`)

| Key | Default | Meaning |
|-----|---------|---------|
| `iotEndpoint` | — | This account's IoT data endpoint (`aws iot describe-endpoint --endpoint-type iot:Data-ATS`) |
| `deviceId` | `nucleo-01` | IoT thing name + MQTT client ID |
| `certificateArn` | — | Device cert ARN — the cert itself is made and kept outside this repo |
| `medThreshold` / `highThreshold` | 100 / 300 | Peak level for MED / HIGH *(to calibrate)* |
| `maxLevel` | 1 | Highest level the cloud will send (keep at 1 until 100 MHz is verified) |
| `holdSeconds` | 15 | How long the board holds a raised clock |

See [docs/protocol.md](docs/protocol.md) for the message formats.
