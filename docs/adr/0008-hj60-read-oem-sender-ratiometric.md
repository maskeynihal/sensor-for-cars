# 0008. hj60_coolant: read the OEM gauge sender ratiometrically

- Status: Proposed
- Date: 2026-11-01
- Projects: hj60_coolant

## Context

- The HJ60 temperature gauge is a bimetal receiver (E–F 25 Ω) fed from a bimetal voltage regulator inside the fuel
  gauge, after a 24 V dropping resistor (FSM 13-73/13-74). The feed **pulses**. The FSM tests it with a flashing bulb.
- Owner measurements: ~10 V at the sender wire for a few seconds after key-on, then a wildly fluctuating DC reading.
  That is the regulator cycling.
- The sender is an NTC: 226 Ω at 50 °C, 26 Ω at 115 °C (FSM). It must keep driving the dash gauge.
- There is no ECU or OBD on a 2H.

## Options

1. **Measure sender voltage only** and assume a fixed feed: wrong whenever the regulator pulses or battery voltage
   changes.
2. **Heavy RC filter on the sender voltage**: averages the pulses, but the average depends on the regulator's duty,
   which drifts with temperature and battery voltage.
3. **Ratiometric: sample the feed (E) and the sender (F) together** through matched high-impedance dividers.
   `Rs = Rg × Vs / (Vfeed − Vs)`. Independent of feed level and pulsing. Needs two ADC channels and a known Rg.
4. **Replace the sender with a dual-output one, or add our own NTC sender**: no published dual sender found for the 2H.
   An extra sender needs a spare port or a hose adapter.
5. **Disconnect the gauge and drive it from the ESP32**: breaks the factory gauge if the ESP32 fails. Rejected.

## Decision

Proposed: option 3 as the primary reading. Measure Rg on the truck and calibrate the sender curve before trusting
absolute values. Keep option 4 (a PTFE DS18B20 in an upper-hose thermowell) as the planned cross-check for coolant loss.

## Consequences

- Two 120 kΩ / 10 kΩ 0.1% dividers with 100 nF and BAT54S clamps; ADC1 pins GPIO34/35.
- Firmware gates on "feed on", rejects pairs across a contact edge, uses the median over 1 s.
- An open sender, a shorted sender, or no feed is a fault, not "cold".
- Accuracy depends on Rg (1 Ω ≈ 4% in Rs), channel matching, and the engine-to-cabin ground offset.
- Accept after the truck measurements M1 to M5 in the report agree with a reference thermometer.
