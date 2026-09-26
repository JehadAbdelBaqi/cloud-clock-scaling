import pytest

from protocol import Ack, Reading, format_command, parse_line


@pytest.mark.parametrize(
    "line, expected",
    [
        ("S,42,318\n", Reading(seq=42, peak=318)),
        ("S,0,0\r\n", Reading(seq=0, peak=0)),
        ("A,42,50\n", Ack(seq=42, mhz=50)),
        ("A,1,16", Ack(seq=1, mhz=16)),
    ],
)
def test_parse_valid(line, expected):
    assert parse_line(line) == expected


@pytest.mark.parametrize(
    "line",
    [
        "",
        "\n",
        "hello from firmware\n",
        "S,42\n",          # missing field
        "S,42,318,9\n",    # extra field
        "S,x,318\n",       # not a number
        "S,-1,318\n",      # negative
        "A,1,33\n",        # not a real clock level
        ",42,318\n",       # half a line at startup
    ],
)
def test_parse_ignores_junk(line):
    assert parse_line(line) is None


def test_format_command():
    assert format_command(42, 1, 15) == "C,42,1,15\n"


@pytest.mark.parametrize("seq, level, hold", [(1, 3, 15), (1, -1, 15), (-1, 1, 15), (1, 1, -5)])
def test_format_command_rejects_bad(seq, level, hold):
    with pytest.raises(ValueError):
        format_command(seq, level, hold)
