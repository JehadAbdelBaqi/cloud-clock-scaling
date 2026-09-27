# Wi-Fi Bridge

The Nucleo has no Wi-Fi, so a small ESP32-S3 board sits between it and AWS:
text lines in over UART, MQTT messages out over Wi-Fi — and the reverse.
This project treats it as a **black box**: the firmware only depends on its
interface (wiring + the line format in [protocol.md](protocol.md)). The bridge's
code lives in its own project, not in this repo.

```
Nucleo ──UART lines──► Wi-Fi bridge ──Wi-Fi + TLS──► AWS IoT Core
       ◄──────────────               ◄─────────────  MQTT, port 8883
```

## Hardware

| Part | Detail |
|------|--------|
| Board | Axiometa **Genesis Mini** |
| Module | Espressif **ESP32-S3-MINI-1** (N4R2: 4 MB flash, 2 MB PSRAM) |
| Wi-Fi | 2.4 GHz only |
| Logic level | 3.3 V — wires straight to the Nucleo |
| Status LED | On-board RGB LED (GPIO21) |

## Why not ESP-AT

The first plan was **ESP-AT**: Espressif's ready-made firmware that turns an
ESP chip into a plain Wi-Fi module driven by AT commands — flash it, no code
of our own. It **doesn't support the ESP32-S3**: there's no build for it and
Espressif's team has said there are no plans to add one
([esp-at #607](https://github.com/espressif/esp-at/issues/607),
[#956](https://github.com/espressif/esp-at/issues/956)) — they point to the
cheaper ESP32-C series for that job.

So the bridge is a short Arduino sketch instead, using Espressif's standard
libraries — no low-level Wi-Fi or TLS code.

## Software

| Piece | Use |
|-------|-----|
| [PlatformIO](https://platformio.org) + Arduino core for ESP32 | Build / upload |
| `WiFi` | Join the network |
| `WiFiClientSecure` | TLS to IoT Core, with the device certificate |
| [PubSubClient](https://github.com/knolleary/pubsubclient) | MQTT: publish readings, subscribe to commands |
| [ArduinoJson](https://arduinojson.org) | Read the command JSON |

## What it does

1. **Join Wi-Fi**, then **connect to IoT Core** over TLS on port 8883 as
   MQTT client `nucleo-01` — the only client ID the device policy allows.
   Retries every 5 s until connected.
2. **Subscribe** to `clockscale/nucleo-01/commands` — again on every
   reconnect, as subscriptions don't survive a dropped connection.
3. **Nucleo → cloud:** reads lines from the Nucleo; `S,<seq>,<peak>` becomes
   `{"seq":…,"peak":…}` on `clockscale/nucleo-01/readings` (which runs the
   Lambda). Anything else — e.g. the Nucleo's `A` acks — is ignored.
4. **Cloud → Nucleo:** each command `{"seq":…,"level":…}` becomes a line
   `C,<seq>,<level>` sent to the Nucleo.
5. **Keeps the connection alive** and reconnects if it drops.

### Status LED

| LED | Means |
|-----|-------|
| Solid **red** | Joining Wi-Fi / connecting to AWS |
| **Blue** blink | Reading sent to AWS |
| **Green** blink | Command received from AWS, passed to the Nucleo |

A loud noise shows as blue then green a moment later — the round trip.

## Connection to the Nucleo

UART (`Serial1`), 115200 baud 8N1, on the Genesis Mini's Port 2:

| Bridge | Nucleo |
|--------|--------|
| GPIO7 (TX) | D2 = PA10 (USART1 RX) |
| GPIO6 (RX) | D8 = PA9 (USART1 TX) |
| GND | GND |

## Device identity (X.509)

- The device certificate and private key were created with
  `aws iot create-keys-and-certificate` by a script in the bridge project.
- They live **only** in the bridge project and on the board. **This repo
  holds only the certificate's ARN** (`infra/cdk/cdk.json`), which isn't a
  secret — CDK uses it to attach the certificate to the thing and its policy.
- The bridge also carries Amazon's root CA, so it only trusts the real AWS
  endpoint.

## Setup notes (Genesis Mini)

| Setting | Why |
|---------|-----|
| 4 MB flash + default partition table | The generic ESP32-S3 board default is 8 MB — on this 4 MB module it boot-loops |
| `ARDUINO_USB_MODE=1` (USB-Serial/JTAG) | One fixed COM port, upload without holding BOOT. (The other mode, for USB mouse/keyboard, moves the COM port after upload) |
| `monitor_dtr = 0`, `monitor_rts = 0` | Stops the serial monitor holding the chip in reset / download mode |

## How it was tested

Built and checked one step at a time, first with the bridge's USB serial
monitor standing in for the Nucleo (typing its lines by hand) and the AWS
console's MQTT test client (subscribed to `clockscale/#`) showing what
reached AWS:

| Step | Check |
|------|-------|
| Wi-Fi | IP address printed |
| TLS + MQTT | `MQTT connected` as `nucleo-01` |
| Publish | Test message seen in the MQTT test client |
| Reading in | Typed `S,1,500` → `{"seq": 1, "peak": 500}` on `readings` → the Lambda's command on `commands` |
| Command out | The command came back out of the bridge as a `C` line |

Then wired to the Nucleo: the full loop — mic → Nucleo → bridge → AWS →
bridge → Nucleo clock switch — confirmed working.

## Related

- [protocol.md](protocol.md) — the line and MQTT message formats
- [README](../README.md#wiring) — wiring
- [decisions.md](decisions.md) — why the bridge is a black box, why not ESP-AT
