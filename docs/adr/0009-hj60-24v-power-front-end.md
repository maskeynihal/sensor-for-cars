# 0009. hj60_coolant: 24 V automotive power front end

- Status: Proposed
- Date: 2026-11-01
- Projects: hj60_coolant

## Context

- The HJ60 is 24 V (two batteries in series). Charging is about 28–29 V.
- ISO 16750-2 load dump for 24 V: Test A (unsuppressed) 151–202 V, Test B (clamped) 58 V. ISO 7637-2 24 V pulses:
  1 = −300/−600 V, 2a = +37/+112 V, 3a = −150/−300 V, 3b = +150/+300 V (vendor app notes; see the report's sources).
- ESP32 DevKit V1 takes 5 V on VIN/5V. The UNO R4 WiFi's VIN max is 24 V, so it can't go on this truck directly
  either (Arduino ABX00087 datasheet).
- The truck has an ignition relay; the gauge circuit is on the 15 A ENGINE fuse.

## Options

1. **Cigarette-lighter 24 V → USB adapter**: easy, but quality and transient handling unknown; plug can fall out.
2. **Discrete front end**: fuse, reverse protection, TVS, a buck rated for ≥60 V input, 5 V out.
3. **Off-the-shelf automotive 24 V → 5 V converter (marine or truck rated)**: fine if it states ≥60 V transient
   tolerance and an operating range covering 18–32 V.

## Decision

Proposed: option 2 or 3. Requirements, whichever is built or bought:

- 1 A fuse at an ignition-switched tap.
- Reverse-polarity protection (Schottky or ideal diode).
- TVS on the input, standoff above 32 V.
- Buck converter, input rated at least 60 V, output 5 V at 500 mA or more, into the DevKit's 5V pin (not 3V3).
- Ground at the instrument cluster, the same point the ADC taps reference.

## Consequences

- No more USB power in the truck.
- The ESP32 powers down with the ignition. Logging (later) needs an ignition-sense input and hold-up capacitance.
- Accept after a bench test at 18 V, 24 V, 32 V and a cranking dip.
