import pytest

from levels import HIGH, LOW, MED, build_command, parse_reading, pick_level


@pytest.mark.parametrize(
    "peak, expected",
    [
        (0, LOW),
        (99, LOW),
        (100, MED),
        (299, MED),
        (300, HIGH),
        (4095, HIGH),
    ],
)
def test_pick_level_thresholds(peak, expected):
    assert pick_level(peak, 100, 300) == expected


def test_pick_level_capped_by_max_level():
    # HIGH stays off until 100 MHz is verified on the board.
    assert pick_level(4095, 100, 300, max_level=MED) == MED


def test_pick_level_rejects_bad_thresholds():
    with pytest.raises(ValueError):
        pick_level(50, 300, 100)


def test_parse_reading_ok():
    r = parse_reading({"seq": "42", "peak": 318, "ts": 1790000000123, "device_id": "nucleo-01"})
    assert r == {"device_id": "nucleo-01", "seq": 42, "peak": 318, "ts": 1790000000123}


def test_parse_reading_ts_optional():
    assert parse_reading({"seq": 1, "peak": 5, "device_id": "nucleo-01"})["ts"] is None


@pytest.mark.parametrize(
    "event",
    [
        {},
        {"seq": 1, "peak": 5},  # no device_id
        {"seq": "x", "peak": 5, "device_id": "nucleo-01"},
        {"seq": 1, "peak": -1, "device_id": "nucleo-01"},
        {"seq": 1, "peak": 5, "device_id": "bad/id"},
    ],
)
def test_parse_reading_rejects_bad_input(event):
    with pytest.raises(ValueError):
        parse_reading(event)


def test_build_command():
    assert build_command(42, MED) == {"seq": 42, "level": 1}
