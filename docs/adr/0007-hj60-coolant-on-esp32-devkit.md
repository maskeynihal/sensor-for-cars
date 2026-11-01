# 0007. hj60_coolant: new project on ESP32 DevKit V1

- Status: Accepted
- Date: 2026-11-01
- Projects: hj60_coolant

## Context

- The target is now known: engine coolant temperature in a Toyota Land Cruiser HJ60 (2H diesel, 24 V system), read
  from the truck's own gauge sender. See [gauge-tap-report](../../projects/hj60_coolant/docs/gauge-tap-report.md).
- water_monitor (UNO R4 WiFi) was built for a water tank at 35/40 °C. Its firmware must stay uploadable to the R4
  (AGENTS.md rule 1).
- The owner has an ESP32 DevKit V1 (WROOM-32). Wokwi runs the ESP32 natively, so the sim board equals the real board,
  unlike water_monitor ([ADR 0006](0006-water-monitor-sim-on-avr-uno.md)).
- The ESP32 has 12-bit ADC1 with factory calibration (`analogReadMilliVolts`), WiFi/BLE for later configuration and
  logging, NVS for settings.

## Options

1. **New project `hj60_coolant` on ESP32 DevKit V1**: water_monitor stays as the bench prototype. Same board in sim
   and on the truck.
2. **Convert water_monitor to ESP32**: one project, but rewrites an uploadable, working sketch and its history.

## Decision

Option 1 (the owner chose it). FQBN and sim FQBN `esp32:esp32:esp32`; Wokwi part `board-esp32-devkit-c-v4`.

## Consequences

- ADC inputs are 3.3 V max and non-linear near the rails. Keep signals in about 0.15 to 2.45 V (11 dB attenuation), and
  use ADC1 only (ADC2 is unusable while WiFi is on).
- GPIO34/35 are input-only, so they are used for the two analog taps.
- The native mocks gained ESP32 ADC calls (`analogReadMilliVolts`, attenuation/resolution stubs, a time-varying
  `sim::analogMvFn`).
- The SH1106 → SSD1306 shim from [ADR 0005](0005-sim-display-shim-sh1106.md) is copied into this project.

## Note (2026-11-01, after acceptance)

The Context says "24 V system". The owner later measured a single 12 V battery (12.9 V off, 14.0 V running). The
decision (ESP32 DevKit V1, new project) doesn't depend on it. Power is covered in
[ADR 0009](0009-hj60-12v-power-front-end.md).
