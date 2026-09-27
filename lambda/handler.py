"""Decision Lambda — invoked by the IoT topic rule for every reading.

reading -> pick clock level -> (if above LOW) publish command -> log one line

The command is published first: that's the fast path the board is waiting
on. The log line comes after and goes to CloudWatch Logs, which is the
reading history (query it with Logs Insights).

LOW is never sent: dropping back to low speed is the board's own job (its
hold timer), so it still gets there if the cloud or network is down.
"""
import json
import logging
import os
import time

import boto3

from levels import CLOCK_MHZ, LOW, build_command, parse_reading, pick_level

log = logging.getLogger()
log.setLevel(logging.INFO)

_iot = None


def _iot_client():
    """Created lazily so the module imports cleanly in unit tests."""
    global _iot
    if _iot is None:
        _iot = boto3.client("iot-data", endpoint_url=f"https://{os.environ['IOT_ENDPOINT']}")
    return _iot


def _settings() -> dict:
    return {
        "topic_root": os.environ.get("TOPIC_ROOT", "clockscale"),
        "med": int(os.environ.get("MED_THRESHOLD", "100")),
        "high": int(os.environ.get("HIGH_THRESHOLD", "300")),
        "max_level": int(os.environ.get("MAX_LEVEL", "1")),
    }


def handler(event, context):
    received_ms = int(time.time() * 1000)
    cfg = _settings()

    try:
        reading = parse_reading(event)
    except ValueError as exc:
        # Bad input from a device shouldn't retry forever — log and drop.
        log.warning("dropping reading: %s", exc)
        return {"status": "dropped", "reason": str(exc)}

    level = pick_level(reading["peak"], cfg["med"], cfg["high"], cfg["max_level"])

    command = None
    if level > LOW:
        command = build_command(reading["seq"], level)
        topic = f"{cfg['topic_root']}/{reading['device_id']}/commands"
        _iot_client().publish(topic=topic, qos=1, payload=json.dumps(command))

    result = {
        "status": "ok",
        "device_id": reading["device_id"],
        "seq": reading["seq"],
        "peak": reading["peak"],
        "level": level,
        "mhz": CLOCK_MHZ[level],
        "commanded": command is not None,
        "gateway_ts": reading["ts"],
        "lambda_received_ms": received_ms,
    }
    # One JSON line per reading — this is the history.
    log.info(json.dumps(result))
    return result
