# 0009. hj60_coolant: 12 V automotive power front end

- Status: Proposed
- Date: 2026-11-01
- Projects: hj60_coolant

## Context

- This HJ60 is **12 V with a single battery**: 12.9 V engine off, 14.0 V running (owner measurement, 2026-11-01). An
  earlier draft of this ADR assumed 24 V. Some HJ60 markets are 24 V; this truck isn't.
- 12 V transients (vendor app notes quoting ISO 16750-2 / ISO 7637-2; links in the
  [report](../../projects/hj60_coolant/docs/gauge-tap-report.md#42-power-12-v-truck--5-v-esp32)):
  - load dump Test A (unsuppressed) 79–101 V, Test B (clamped) 35 V;
  - cranking dips to about 6 V;
  - jump start 24–26 V for 60 s;
  - reverse battery −14 V for 60 s;
  - pulse 1 −75/−100 V, pulse 3b +75/+150 V.
- ESP32 DevKit V1 takes 5 V on VIN/5V. The UNO R4 WiFi's VIN (6–24 V) only has Schottky diodes for reverse polarity
  and overvoltage, with no TVS (Arduino ABX00087 datasheet).
- The truck has an ignition relay; the gauge circuit is on the 15 A ENGINE fuse.

## Options

1. **Cigarette-lighter 12 V → USB adapter**: easy, but quality and transient handling unknown; plug can fall out.
2. **Discrete front end**: fuse, reverse protection, TVS, a wide-input buck, 5 V out.
3. **Off-the-shelf automotive 12 V → 5 V converter**: fine if it states load-dump tolerance (≥ 40 V, ideally 60 V)
   and an operating range down to about 6 V.

## Decision

Proposed: option 2 or 3. Requirements, whichever is built or bought:

- 1 A fuse at an ignition-switched tap.
- Reverse-polarity protection (Schottky or ideal diode).
- TVS on the input, standoff about 26–28 V (survives a jump start), clamp below the converter's maximum input.
- Buck converter, input rated ≥ 40 V (60 V preferred), regulating down to about 6 V in, 5 V at 500 mA or more, into
  the DevKit's 5V pin (not 3V3).
- Ground at the instrument cluster, the same point the ADC taps reference.

## Consequences

- No more USB power in the truck.
- The ESP32 powers down with the ignition and may reset during cranking. It reboots into `WAIT` within a second or so.
- Logging (later) needs an ignition-sense input and hold-up capacitance.
- Accept after a bench test at 6 V, 12 V, 14.4 V, 16 V and 26 V (jump start), plus a cranking dip on the truck.
