# Idea: Water + air temperature monitor

- Date: 2026-11-01 (written up after the build)
- Status: Building -> [projects/water_monitor](../../projects/water_monitor/)
- Board: Arduino UNO R4 WiFi

## Problem

Watch water temperature with a probe, plus cabin/ambient air temperature and humidity. Alarm when the water gets
warm (35 °C) or hot (40 °C).

## Sketch of a solution

- Inputs: DS18B20 waterproof probe (OneWire), DHT22 (temp + humidity)
- Outputs: 1.3" SH1106 OLED with an animated LC78 off-road scene, active buzzer
- Power: USB from the car's 5 V
- Where it mounts: TBD

## Open questions

- Answered 2026-11-01: the real target is **engine coolant in an HJ60 Land Cruiser (24 V)**, not a water tank. That
  work moved to [projects/hj60_coolant](../../projects/hj60_coolant/) on an ESP32. water_monitor stays as the bench
  prototype. Why it doesn't fit the truck as is (thresholds, silent probe failure, PVC probe, 24 V VIN limit, Dupont
  wiring, animation): [gauge-tap-report section 5](../../projects/hj60_coolant/docs/gauge-tap-report.md#5-what-we-did-wrong-in-water_monitor-for-this-use).
- Thresholds are fixed in code. Should they be configurable (buttons? WiFi, since the R4 has it)?
- Use the R4 WiFi radio to log to a phone?
- "LC78" on the splash screen: the truck is an HJ60. Rename the art, or keep it?

## Notes / links

- Wiring: [projects/water_monitor/docs/markdown/README.md](../../projects/water_monitor/docs/markdown/README.md)
