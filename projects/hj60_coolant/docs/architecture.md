# hj60_coolant -- software architecture

## Loop

```mermaid
flowchart TD
  S[setup: Serial, pins] --> L{loop}
  L -->|every 1 s| H[toggle STATUS_LED + log heartbeat]
  H --> L
```

## Timing

| Task | Period | Blocking? |
|---|---|---|
| heartbeat | 1000 ms | no (millis) |

## Serial protocol

| Line | Meaning |
|---|---|
| `hj60_coolant boot ok` | setup() finished |
| `heartbeat <s>` | alive, uptime seconds |
