"""Decision Lambda — invoked by the IoT topic rule for every reading.

reading -> pick clock level -> (if above LOW) publish command -> log to DynamoDB

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

HISTORY_TTL_S = 7 * 24 * 3600  # readings auto-expire after a week

_table = None
_iot = None


def _clients():
    """Created lazily so the module imports cleanly in unit tests."""
    global _table, _iot
    if _table is None:
        _table = boto3.resource("dynamodb").Table(os.environ["TABLE_NAME"])
    if _iot is None:
        _iot = boto3.client("iot-data", endpoint_url=f"https://{os.environ['IOT_ENDPOINT']}")
    return _table, _iot


def _settings() -> dict:
    return {
        "topic_root": os.environ.get("TOPIC_ROOT", "clockscale"),
        "med": int(os.environ.get("MED_THRESHOLD", "100")),
        "high": int(os.environ.get("HIGH_THRESHOLD", "300")),
        "max_level": int(os.environ.get("MAX_LEVEL", "1")),
        "hold_s": int(os.environ.get("HOLD_SECONDS", "15")),
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
    table, iot = _clients()

    command = None
    if level > LOW:
        command = build_command(reading["seq"], level, cfg["hold_s"])
        topic = f"{cfg['topic_root']}/{reading['device_id']}/commands"
        iot.publish(topic=topic, qos=1, payload=json.dumps(command))

    table.put_item(
        Item={
            "device_id": reading["device_id"],
            "ts": reading["ts"] or received_ms,
            "seq": reading["seq"],
            "peak": reading["peak"],
            "level": level,
            "mhz": CLOCK_MHZ[level],
            "commanded": command is not None,
            "lambda_received_ms": received_ms,
            "expires_at": received_ms // 1000 + HISTORY_TTL_S,
        }
    )

    result = {
        "status": "ok",
        "device_id": reading["device_id"],
        "seq": reading["seq"],
        "peak": reading["peak"],
        "level": level,
        "mhz": CLOCK_MHZ[level],
        "commanded": command is not None,
    }
    # One JSON line per reading — easy to query in CloudWatch Logs Insights.
    log.info(json.dumps(result))
    return result
