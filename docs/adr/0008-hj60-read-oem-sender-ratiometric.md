# 0008. hj60_coolant: read the OEM gauge sender ratiometrically

- Status: Proposed
- Date: 2026-11-01
- Projects: hj60_coolant

## Context

- The HJ60 temperature gauge is a bimetal receiver (E–F 25 Ω) fed from a bimetal voltage regulator inside the fuel
  gauge (FSM 13-73/13-74). On 24 V trucks a dropping resistor sits before it; this truck is 12 V. The feed **pulses**. The FSM tests it with a flashing bulb.
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
5. **Option B: disconnect the gauge and measure the sender directly** (Rref 100 Ω from a switched 3.3 V). About
   19 mV/°C and a steady signal, but the dash gauge stops working, and an ESP32 failure leaves the driver with no
   temperature. Bench and calibration (M4) use only.
6. **Option C: measure directly, then operate the gauge from the ESP32** (PWM low-side MOSFET + 20 Ω Rmin, duty
   `((Rg+Rmin)/(Rg+Rs))²`), behind a normally closed fail-safe relay that restores the factory circuit when the ESP32
   is off or crashes. Keeps the gauge, best signal, most parts. The regulator's behaviour with a PWM load is untested.
   Details and a comparison table: [report section 8](../../projects/hj60_coolant/docs/gauge-tap-report.md#8-other-connection-options-b-and-c).

## Decision

Proposed: option 3 (option A in the report) as the primary reading. Option 6 (C) is the planned upgrade if option 3
is not accurate enough after M4/M5, and only with the fail-safe relay and a bench test of the regulator. Option 5 (B)
is not for a truck that is driven. Measure Rg on the truck and calibrate the sender curve before trusting
absolute values. Keep option 4 (a PTFE DS18B20 in an upper-hose thermowell) as the planned cross-check for coolant loss.

## Consequences

- Two 68 kΩ / 10 kΩ (7.8:1) 0.1% dividers with 100 nF and BAT54S clamps; ADC1 pins GPIO34/35.
- Firmware gates on "feed on", rejects pairs across a contact edge, uses the median over 1 s.
- An open sender, a shorted sender, or no feed is a fault, not "cold".
- Accuracy depends on Rg (1 Ω ≈ 4% in Rs), channel matching, and the engine-to-cabin ground offset.
- Accept after the truck measurements M1 to M5 in the report agree with a reference thermometer.

## Note (2026-11-01): build-time switch

The owner asked for option 5 (B) in code. The sketch now has `senderMode` (`SENDER_MODE_A` = option 3,
`SENDER_MODE_B` = option 5). The default is B, at the owner's request. Mode B reads Vtop and Vs through matched 47k/47k
dividers (an undivided Vs near 3 V is compressed by the ADC, so an open wire read as "cold"), and checks the wire
with the excitation off (`FAULT-BACKFEED`). This ADR stays Proposed until the truck measurements M1–M5.

