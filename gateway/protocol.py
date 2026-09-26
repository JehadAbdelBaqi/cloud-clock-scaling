"""UART line protocol between the board and the gateway.

Board -> gateway
    S,<seq>,<peak>      sound reading (every 5 s, or immediately on a clap)
    A,<seq>,<mhz>       ack: clock switched, now running at <mhz>

Gateway -> board
    C,<seq>,<level>,<hold_s>   set clock level (0=LOW 1=MED 2=HIGH), hold for hold_s

Any other line (debug prints from the firmware) is passed through as a log.
Full contract: docs/protocol.md
"""
from dataclasses import dataclass

VALID_LEVELS = (0, 1, 2)
VALID_MHZ = (16, 50, 100)


@dataclass(frozen=True)
class Reading:
    seq: int
    peak: int


@dataclass(frozen=True)
class Ack:
    seq: int
    mhz: int


def parse_line(line: str) -> Reading | Ack | None:
    """Parse one line from the board. Returns None for anything that isn't a
    well-formed S/A message (debug output, noise, half a line at startup)."""
    parts = line.strip().split(",")
    if not parts or parts[0] not in ("S", "A"):
        return None
    if len(parts) != 3:
        return None
    try:
        seq, value = int(parts[1]), int(parts[2])
    except ValueError:
        return None
    if seq < 0 or value < 0:
        return None

    if parts[0] == "S":
        return Reading(seq=seq, peak=value)
    if value not in VALID_MHZ:
        return None
    return Ack(seq=seq, mhz=value)


def format_command(seq: int, level: int, hold_s: int) -> str:
    """Build the UART line for a cloud command."""
    if level not in VALID_LEVELS:
        raise ValueError(f"bad level: {level}")
    if seq < 0 or hold_s < 0:
        raise ValueError("seq and hold_s must be >= 0")
    return f"C,{seq},{level},{hold_s}\n"
