# Message Protocol

The contract between the board, the Wi-Fi bridge and the cloud. Firmware and
Lambda (`infra/decide-clock-lambda/levels.py`) must all agree with this file — change it here first.

## Clock levels

| Level | Name | SYSCLK | Source |
|-------|------|--------|--------|
| 0 | LOW | 16 MHz | HSI |
| 1 | MED | 50 MHz | PLL |

`maxLevel` in `infra/cdk/cdk.json` caps what the cloud will send — 1 (MED).

## UART (board ↔ bridge)

ASCII, one message per line, `\n` terminated (`\r\n` tolerated). Baud: 115200
(`UART_BAUD` in the firmware — must match the bridge). Nucleo side: USART1,
TX = PA9 (D8), RX = PA10 (D2). Lines that don't match are treated as debug
output and ignored.

| Line | Direction | Meaning |
|------|-----------|---------|
| `S,<seq>,<peak>` | board → bridge | Sound reading. `peak` = ADC peak-to-peak over one window, in ADC counts (0–4095). Sent every 30 s, and straight away on a loud noise (see [What counts as a "loud noise"](../README.md#what-counts-as-a-loud-noise)) |
| `A,<seq>,<mhz>` | board → bridge | Ack: clock switched, now at `<mhz>`. The bridge doesn't forward it to AWS yet |
| `C,<seq>,<level>` | bridge → board | Switch to `<level>`. The board holds it for its own hold time, then drops back to LOW |

`seq` is set by the board on each reading and echoed back in the command and
ack — it ties the whole round trip together for latency measurement.

## MQTT (bridge ↔ AWS IoT Core)

JSON payloads, QoS 1. `<device_id>` = the IoT thing name (default `nucleo-01`),
also used as the MQTT client ID.

| Topic | Direction | Payload |
|-------|-----------|---------|
| `clockscale/<device_id>/readings` | device → cloud | `{"seq": 42, "peak": 318}` — runs the Lambda |
| `clockscale/<device_id>/commands` | cloud → device | `{"seq": 42, "level": 1}` |
| `clockscale/<device_id>/status` | device → cloud | Allowed by the policy, not used yet |

A reading may also carry `"ts"` (bridge time in ms since epoch when the line
arrived) — the Lambda accepts it but the bridge doesn't send it yet.

## Board behaviour (firmware contract)

- Boots at LOW.
- Sends `S` every `READING_INTERVAL_S` (30 s) with the loudest level since the
  last one — every reading runs the Lambda, so not more often.
- Sends `S` straight away when a window's `peak` reaches `MIC_CLAP_LEVEL` — at
  most once per SysTick countdown, so a long noise can't flood the cloud.
  Keep `MIC_CLAP_LEVEL` equal to the cloud's `medThreshold`.
- On `C`: switch, reply `A`, start (or restart) the hold timer. The hold time
  is the board's own setting (`HOLD_S` in `firmware/include/config/app_config.h`) —
  the cloud only says *speed up*.
- Hold timer expiry → switch to LOW, send `A` with `mhz` = 16 and the last
  `seq`.
- The cloud never sends LOW — dropping back is the board's job, so it still
  happens if the network is down.
- After every switch: recompute anything timed off the bus clock (UART baud
  registers, timer prescalers).
