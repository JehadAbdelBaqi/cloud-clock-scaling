# cdk — Infrastructure as Code

AWS CDK app (Python) — everything the device talks to in AWS, as code. One
stack: `ClockScaleStack`.

## Layout

```
infra/cdk/
├── app.py                 Entry point — reads settings from cdk.json, builds the stack
├── cdk.json               CDK config + this project's settings ("context")
├── IaC/
│   └── infra_stack.py     The stack — all resources below
└── tests/unit/
    └── test_infra_stack.py  Synth the stack and check the resources + permissions
```

## What gets deployed

| Resource | Purpose |
|----------|---------|
| **IoT Thing** (`nucleo-01`) | The device's identity in IoT Core |
| **IoT policy** | What the device may do: connect as `nucleo-01` only, publish to its `readings` / `status` topics, subscribe/receive on its `commands` topic |
| **Certificate attachments** | Attach the device certificate (made outside the stack — CloudFormation can't hand back a private key) to the thing and policy. Only created when `certificateArn` is set |
| **Lambda** `DecideClockFn` | Code in [`../decide-clock-lambda/`](../decide-clock-lambda/README.md) — picks a clock level from each reading, publishes the command |
| **Lambda permission** | Only publish to `clockscale/*/commands` |
| **Log group** | Lambda logs, 1-week retention — one line per reading = the history |
| **IoT topic rule** | `SELECT *, topic(2) AS device_id FROM 'clockscale/+/readings'` → Lambda |

Stack outputs: thing name, policy name, topics, Lambda name, log group name.

## Settings — `cdk.json` → `context`

| Key | Default | Meaning |
|-----|---------|---------|
| `iotEndpoint` | — (required) | The account's IoT data endpoint: `aws iot describe-endpoint --endpoint-type iot:Data-ATS` |
| `deviceId` | `nucleo-01` | Thing name + MQTT client ID |
| `certificateArn` | — | Device certificate ARN (not a secret — the key never comes near this repo) |
| `medThreshold` | 100 | Loudness that gets a MED (50 MHz) command |
| `maxLevel` | 1 | Highest level the cloud sends (1 = MED) |

## Deploy

From this folder, logged in to AWS (`aws sso login`):

```bash
cdk bootstrap                        # once per account/region
cdk deploy ClockScaleStack --require-approval never   # Git Bash has no prompt for approvals
```

## Tests

From the repo root: `uv run pytest` (runs these and the Lambda tests).
