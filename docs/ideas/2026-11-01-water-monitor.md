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

- Thresholds are fixed in code. Should they be configurable (buttons? WiFi, since the R4 has it)?
- 12 V direct supply instead of USB?
- Use the R4 WiFi radio to log to a phone?

## Notes / links

- Wiring: [projects/water_monitor/docs/markdown/README.md](../../projects/water_monitor/docs/markdown/README.md)
