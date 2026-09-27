"""Pure decision logic — no AWS imports, so it's trivially unit-testable.

Clock levels match the firmware:
    0 = LOW  -> 16 MHz  (HSI)
    1 = MED  -> 50 MHz  (PLL)
    2 = HIGH -> 100 MHz (PLL, chip max)
"""
LOW, MED, HIGH = 0, 1, 2

CLOCK_MHZ = {LOW: 16, MED: 50, HIGH: 100}


def pick_level(peak: int, med_threshold: int, high_threshold: int, max_level: int = HIGH) -> int:
    """Map a peak-to-peak sound level to a clock level.

    Fixed steps, not a smooth scale — the PLL only has discrete settings.
    `max_level` caps the result (e.g. keep HIGH off until 100 MHz is verified
    on the board).
    """
    if med_threshold >= high_threshold:
        raise ValueError("med_threshold must be below high_threshold")

    if peak >= high_threshold:
        level = HIGH
    elif peak >= med_threshold:
        level = MED
    else:
        level = LOW

    return min(level, max_level)


def parse_reading(event: dict) -> dict:
    """Validate the reading forwarded by the IoT rule.

    Expected (from the device, plus device_id added by the rule SQL):
        {"seq": 42, "peak": 318, "ts": 1790000000123, "device_id": "nucleo-01"}
    """
    try:
        device_id = str(event["device_id"])
        seq = int(event["seq"])
        peak = int(event["peak"])
    except (KeyError, TypeError, ValueError) as exc:
        raise ValueError(f"bad reading: {event!r}") from exc

    if not device_id or not all(c.isalnum() or c in "-_" for c in device_id):
        raise ValueError(f"bad device_id: {device_id!r}")
    if peak < 0:
        raise ValueError(f"negative peak: {peak}")

    ts = event.get("ts")
    return {
        "device_id": device_id,
        "seq": seq,
        "peak": peak,
        "ts": int(ts) if ts is not None else None,
    }


def build_command(seq: int, level: int) -> dict:
    """Command sent back down to the board. How long to hold is the board's call."""
    return {"seq": seq, "level": level}
