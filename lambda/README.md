# Lambda

The cloud's decision: for every sound reading, pick a clock level and — if it's
above LOW — publish a command back to the device. Python 3.12, deployed by
[`../infra/`](../infra/README.md).

## Layout

```
lambda/
├── handler.py      Entry point: reading in -> level -> command out -> one log line
├── levels.py       Pure decision logic, no AWS imports (easy to unit-test)
└── tests/          pytest — levels + handler (IoT client faked)
```

## Flow

1. The IoT topic rule calls `handler()` with the reading, plus `device_id`
   taken from the topic: `{"seq": 42, "peak": 318, "device_id": "nucleo-01"}`.
2. `parse_reading()` checks it — bad input is logged and dropped (no retries).
3. `pick_level()` maps loudness to a level:

   | Loudness | Level |
   |----------|-------|
   | ≥ `HIGH_THRESHOLD` | HIGH (100 MHz) |
   | ≥ `MED_THRESHOLD` | MED (50 MHz) |
   | below | LOW — no command |

   Capped at `MAX_LEVEL`.
4. Above LOW → publish `{"seq", "level"}` to `clockscale/<device_id>/commands`.
   **LOW is never sent** — dropping back, and how long to hold, is the board's job.
5. Log one JSON line per reading (level, whether a command went out, timings) —
   the reading history, searchable in CloudWatch Logs Insights.

## Environment (set by the stack from `cdk.json`)

| Variable | Meaning |
|----------|---------|
| `IOT_ENDPOINT` | Where to publish commands |
| `TOPIC_ROOT` | `clockscale` |
| `MED_THRESHOLD` / `HIGH_THRESHOLD` | Loudness thresholds |
| `MAX_LEVEL` | Highest level allowed |

## Tests

From the repo root: `uv run pytest`.
