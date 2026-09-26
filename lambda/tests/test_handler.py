"""Handler tests with the IoT client swapped for a fake — no AWS needed."""
import json

import pytest

import handler


class FakeIot:
    def __init__(self):
        self.published = []

    def publish(self, topic, qos, payload):
        self.published.append({"topic": topic, "qos": qos, "payload": json.loads(payload)})


@pytest.fixture
def iot(monkeypatch):
    fake = FakeIot()
    monkeypatch.setattr(handler, "_iot_client", lambda: fake)
    monkeypatch.setenv("MED_THRESHOLD", "100")
    monkeypatch.setenv("HIGH_THRESHOLD", "300")
    monkeypatch.setenv("MAX_LEVEL", "2")
    monkeypatch.setenv("HOLD_SECONDS", "15")
    return fake


def reading(peak, seq=7):
    return {"seq": seq, "peak": peak, "ts": 1790000000000, "device_id": "nucleo-01"}


def test_quiet_reading_sends_no_command(iot):
    result = handler.handler(reading(20), None)
    assert result["level"] == 0 and result["mhz"] == 16
    assert not result["commanded"]
    assert iot.published == []


def test_clap_publishes_command_to_device_topic(iot):
    result = handler.handler(reading(150, seq=9), None)
    assert result["mhz"] == 50 and result["commanded"]
    assert iot.published == [
        {
            "topic": "clockscale/nucleo-01/commands",
            "qos": 1,
            "payload": {"seq": 9, "level": 1, "hold_s": 15},
        }
    ]


def test_very_loud_goes_high_when_allowed(iot):
    handler.handler(reading(900), None)
    assert iot.published[0]["payload"]["level"] == 2


def test_logs_one_json_line_per_reading(iot, caplog):
    caplog.set_level("INFO")
    handler.handler(reading(150, seq=3), None)
    lines = [json.loads(r.message) for r in caplog.records if r.message.startswith("{")]
    assert len(lines) == 1
    assert lines[0]["seq"] == 3 and lines[0]["mhz"] == 50


def test_bad_reading_dropped_without_aws_calls(iot):
    result = handler.handler({"peak": 5}, None)
    assert result["status"] == "dropped"
    assert iot.published == []
