# Message Protocol

The contract between the board, the gateway and the cloud. Firmware and Lambda (`lambda/levels.py`) must all agree with
this file — change it here first.

## Clock levels

| Level | Name | SYSCLK | Source |
|-------|------|--------|--------|
| 0 | LOW | 16 MHz | HSI |
| 1 | MED | 50 MHz | PLL |
| 2 | HIGH | 100 MHz | PLL |

`maxLevel` in `infra/cdk.json` caps what the cloud will send — keep it at 1
until 100 MHz is verified on the board.

## UART (board ↔ gateway)

ASCII, one message per line, `\n` terminated (`\r\n` tolerated). Baud: 115200
(must match the firmware). Lines that don't match are treated as debug output
and ignored.

| Line | Direction | Meaning |
|------|-----------|---------|
| `S,<seq>,<peak>` | board → gateway | Sound reading. `peak` = ADC peak-to-peak over the sample window. Sent every 5 s, and immediately on a clap |
| `A,<seq>,<mhz>` | board → gateway | Ack: clock switched, now at `<mhz>` (16/50/100) |
| `C,<seq>,<level>` | gateway → board | Switch to `<level>`. The board holds it for its own hold time, then drops back to LOW |

`seq` is set by the board on each reading and echoed back in the command and
ack — it ties the whole round trip together for latency measurement.

## MQTT (gateway ↔ AWS IoT Core)

JSON payloads, QoS 1. `<device_id>` = the IoT thing name (default `nucleo-01`),
also used as the MQTT client ID.

| Topic | Direction | Payload |
|-------|-----------|---------|
| `clockscale/<device_id>/readings` | device → cloud | `{"seq": 42, "peak": 318, "ts": 1790000000123}` |
| `clockscale/<device_id>/commands` | cloud → device | `{"seq": 42, "level": 1}` |
| `clockscale/<device_id>/status` | device → cloud | `{"seq": 42, "mhz": 50, "ts": 1790000000456}` |

`ts` = gateway time in ms since epoch when the line arrived from the board.

## Board behaviour (firmware contract)

- Boots at LOW.
- Sends `S` every 5 s; sends `S` immediately when `peak` crosses the clap
  threshold.
- On `C`: queue it if a switch is in progress, then switch, reply `A`, start
  (or restart) the hold timer. The hold time is the board's own setting
  (`HOLD_S` in `firmware/include/app_config.h`) — the cloud only says *speed up*.
- Hold timer expiry → switch to LOW, send `A` with `mhz` = 16 and the last
  `seq`.
- The cloud never sends LOW — dropping back is the board's job, so it still
  happens if the network is down.
- After every switch: recompute anything timed off the bus clock (UART baud
  register, timer prescalers).
