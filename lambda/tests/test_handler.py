"""Handler tests with the AWS clients swapped for fakes — no AWS needed."""
import json

import pytest

import handler


class FakeTable:
    def __init__(self):
        self.items = []

    def put_item(self, Item):
        self.items.append(Item)


class FakeIot:
    def __init__(self):
        self.published = []

    def publish(self, topic, qos, payload):
        self.published.append({"topic": topic, "qos": qos, "payload": json.loads(payload)})


@pytest.fixture
def fakes(monkeypatch):
    table, iot = FakeTable(), FakeIot()
    monkeypatch.setattr(handler, "_clients", lambda: (table, iot))
    monkeypatch.setenv("MED_THRESHOLD", "100")
    monkeypatch.setenv("HIGH_THRESHOLD", "300")
    monkeypatch.setenv("MAX_LEVEL", "2")
    monkeypatch.setenv("HOLD_SECONDS", "15")
    return table, iot


def reading(peak, seq=7):
    return {"seq": seq, "peak": peak, "ts": 1790000000000, "device_id": "nucleo-01"}


def test_quiet_reading_logs_but_sends_no_command(fakes):
    table, iot = fakes
    result = handler.handler(reading(20), None)
    assert result["level"] == 0 and not result["commanded"]
    assert iot.published == []
    assert table.items[0]["mhz"] == 16


def test_clap_publishes_command_to_device_topic(fakes):
    table, iot = fakes
    result = handler.handler(reading(150, seq=9), None)
    assert result["mhz"] == 50
    assert iot.published == [
        {
            "topic": "clockscale/nucleo-01/commands",
            "qos": 1,
            "payload": {"seq": 9, "level": 1, "hold_s": 15},
        }
    ]
    assert table.items[0]["commanded"] is True


def test_very_loud_goes_high_when_allowed(fakes):
    _, iot = fakes
    handler.handler(reading(900), None)
    assert iot.published[0]["payload"]["level"] == 2


def test_bad_reading_dropped_without_aws_calls(fakes):
    table, iot = fakes
    result = handler.handler({"peak": 5}, None)
    assert result["status"] == "dropped"
    assert table.items == [] and iot.published == []
