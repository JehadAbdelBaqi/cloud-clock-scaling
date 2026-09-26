#!/usr/bin/env python3
"""Serial <-> AWS IoT Core bridge.

The Nucleo has no network hardware, so this script is its link to the cloud:

    board --UART--> gateway --MQTT/TLS--> IoT Core    (readings, acks)
    board <--UART-- gateway <--MQTT/TLS-- IoT Core    (commands)

Modes
-----
    python gateway.py --port COM5 --endpoint xxxx-ats.iot.eu-west-2.amazonaws.com
    python gateway.py --simulate --endpoint ...      # fake board, real AWS
    python gateway.py --simulate --dry-run           # fake board, no AWS at all

Every hop is logged with a millisecond timestamp for the latency table.
"""
import argparse
import json
import logging
import random
import sys
import threading
import time
from pathlib import Path

from protocol import Ack, Reading, format_command, parse_line

log = logging.getLogger("gateway")


def now_ms() -> int:
    return int(time.time() * 1000)


# --------------------------------------------------------------------------
# Board link: real serial port, or a simulated board for testing
# --------------------------------------------------------------------------
class SerialBoard:
    def __init__(self, port: str, baud: int):
        import serial  # imported here so --simulate works without pyserial

        self._ser = serial.Serial(port, baud, timeout=1)
        self._lock = threading.Lock()

    def readline(self) -> str:
        return self._ser.readline().decode("ascii", errors="replace")

    def write(self, line: str) -> None:
        with self._lock:
            self._ser.write(line.encode("ascii"))


class SimulatedBoard:
    """Behaves like the firmware: quiet readings every 5 s, random claps sent
    immediately, acks commands, drops back to 16 MHz after the hold time."""

    MHZ = {0: 16, 1: 50, 2: 100}

    def __init__(self, heartbeat_s: float = 5.0, clap_chance: float = 0.15):
        self.heartbeat_s = heartbeat_s
        self.clap_chance = clap_chance
        self._seq = 0
        self._mhz = 16
        self._hold_until = 0.0
        self._outbox: list[str] = []
        self._lock = threading.Lock()

    def readline(self) -> str:
        with self._lock:
            if self._outbox:
                return self._outbox.pop(0)
            if self._mhz != 16 and time.time() >= self._hold_until:
                self._mhz = 16
                log.info("[sim] hold expired -> 16 MHz")
                return f"A,{self._seq},16\n"

        time.sleep(self.heartbeat_s)
        self._seq += 1
        if random.random() < self.clap_chance:
            peak = random.randint(120, 600)
            log.info("[sim] *clap*")
        else:
            peak = random.randint(5, 60)
        return f"S,{self._seq},{peak}\n"

    def write(self, line: str) -> None:
        parts = line.strip().split(",")
        if parts[0] != "C" or len(parts) != 4:
            return
        seq, level, hold_s = int(parts[1]), int(parts[2]), int(parts[3])
        with self._lock:
            self._mhz = self.MHZ.get(level, 16)
            self._hold_until = time.time() + hold_s
            self._outbox.append(f"A,{seq},{self._mhz}\n")
        log.info("[sim] clock -> %d MHz for %d s", self._mhz, hold_s)


# --------------------------------------------------------------------------
# Cloud link: AWS IoT Core over MQTT (mutual TLS), or a dry-run stub
# --------------------------------------------------------------------------
class IotLink:
    def __init__(self, endpoint: str, client_id: str, certs_dir: Path):
        from awscrt import mqtt
        from awsiot import mqtt_connection_builder

        self._qos = mqtt.QoS.AT_LEAST_ONCE
        self._conn = mqtt_connection_builder.mtls_from_path(
            endpoint=endpoint,
            cert_filepath=str(certs_dir / "device.pem.crt"),
            pri_key_filepath=str(certs_dir / "private.pem.key"),
            ca_filepath=str(certs_dir / "AmazonRootCA1.pem"),
            client_id=client_id,  # must match the IoT policy (client/<device_id>)
            clean_session=False,
            keep_alive_secs=30,
        )

    def connect(self) -> None:
        self._conn.connect().result()
        log.info("connected to IoT Core")

    def publish(self, topic: str, payload: dict) -> None:
        self._conn.publish(topic=topic, payload=json.dumps(payload), qos=self._qos)

    def subscribe(self, topic: str, on_message) -> None:
        def _cb(topic, payload, dup, qos, retain, **kwargs):
            on_message(json.loads(payload))

        future, _ = self._conn.subscribe(topic=topic, qos=self._qos, callback=_cb)
        future.result()
        log.info("subscribed to %s", topic)

    def disconnect(self) -> None:
        self._conn.disconnect().result()


class DryRunLink:
    """No AWS: prints what would be published, and plays the Lambda's part
    locally so the full loop can be exercised offline."""

    def __init__(self, med: int = 100, high: int = 300, max_level: int = 1, hold_s: int = 15):
        self._on_message = None
        self._med, self._high, self._max, self._hold = med, high, max_level, hold_s

    def connect(self) -> None:
        log.info("[dry-run] not connecting to AWS")

    def publish(self, topic: str, payload: dict) -> None:
        log.info("[dry-run] publish %s %s", topic, payload)
        if topic.endswith("/readings") and self._on_message:
            peak = payload["peak"]
            level = 2 if peak >= self._high else 1 if peak >= self._med else 0
            level = min(level, self._max)
            if level:
                self._on_message({"seq": payload["seq"], "level": level, "hold_s": self._hold})

    def subscribe(self, topic: str, on_message) -> None:
        self._on_message = on_message

    def disconnect(self) -> None:
        pass


# --------------------------------------------------------------------------
# Bridge
# --------------------------------------------------------------------------
def run(board, cloud, device_id: str, topic_root: str) -> None:
    readings_topic = f"{topic_root}/{device_id}/readings"
    status_topic = f"{topic_root}/{device_id}/status"
    commands_topic = f"{topic_root}/{device_id}/commands"

    def on_command(cmd: dict) -> None:
        try:
            line = format_command(int(cmd["seq"]), int(cmd["level"]), int(cmd["hold_s"]))
        except (KeyError, TypeError, ValueError) as exc:
            log.warning("ignoring bad command %r: %s", cmd, exc)
            return
        log.info("cloud->board  t=%d  %s", now_ms(), line.strip())
        board.write(line)

    cloud.connect()
    cloud.subscribe(commands_topic, on_command)

    log.info("bridging %s <-> %s", device_id, topic_root)
    try:
        while True:
            line = board.readline()
            if not line:
                continue
            msg = parse_line(line)
            t = now_ms()
            if isinstance(msg, Reading):
                log.info("board->cloud  t=%d  seq=%d peak=%d", t, msg.seq, msg.peak)
                cloud.publish(readings_topic, {"seq": msg.seq, "peak": msg.peak, "ts": t})
            elif isinstance(msg, Ack):
                log.info("board ack     t=%d  seq=%d now %d MHz", t, msg.seq, msg.mhz)
                cloud.publish(status_topic, {"seq": msg.seq, "mhz": msg.mhz, "ts": t})
            else:
                log.debug("board says: %s", line.strip())
    except KeyboardInterrupt:
        log.info("stopping")
    finally:
        cloud.disconnect()


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--port", help="serial port, e.g. COM5 (not needed with --simulate)")
    p.add_argument("--baud", type=int, default=115200, help="must match the firmware")
    p.add_argument("--endpoint", help="IoT data endpoint (stack output IotEndpoint)")
    p.add_argument("--device-id", default="nucleo-01")
    p.add_argument("--topic-root", default="clockscale")
    p.add_argument("--certs-dir", type=Path, default=Path(__file__).resolve().parent.parent / "certs")
    p.add_argument("--simulate", action="store_true", help="fake board instead of a serial port")
    p.add_argument("--dry-run", action="store_true", help="don't connect to AWS")
    p.add_argument("-v", "--verbose", action="store_true")
    args = p.parse_args(argv)

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s %(levelname)-5s %(message)s",
    )

    if not args.simulate and not args.port:
        p.error("--port is required unless --simulate")
    if not args.dry_run and not args.endpoint:
        p.error("--endpoint is required unless --dry-run")

    board = SimulatedBoard() if args.simulate else SerialBoard(args.port, args.baud)
    cloud = DryRunLink() if args.dry_run else IotLink(args.endpoint, args.device_id, args.certs_dir)

    run(board, cloud, args.device_id, args.topic_root)
    return 0


if __name__ == "__main__":
    sys.exit(main())
