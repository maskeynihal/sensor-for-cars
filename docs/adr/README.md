# Architecture Decision Records

Format: [0000-template.md](0000-template.md). New ADR: `/adr <title>` (Claude Code), or copy the template.

| # | Decision | Status | Scope |
|---|---|---|---|
| [0001](0001-record-architecture-decisions.md) | Record architecture decisions | Accepted | all |
| [0002](0002-arduino-cli-and-vendored-libraries.md) | arduino-cli with vendored `libraries/` | Accepted | all |
| [0003](0003-projects-layout-firmware-vs-emulator.md) | `projects/<name>/` with separate firmware + emulator copies | Accepted | all |
| [0004](0004-two-tier-emulation.md) | Native host tests + Wokwi | Accepted | all |
| [0005](0005-sim-display-shim-sh1106.md) | SH1106 → SSD1306 shim for Wokwi | Accepted | water_monitor |
| [0006](0006-water-monitor-sim-on-avr-uno.md) | water_monitor sim on AVR Uno | Accepted | water_monitor |
| [0007](0007-hj60-coolant-on-esp32-devkit.md) | hj60_coolant on ESP32 DevKit V1 | Accepted | hj60_coolant |
| [0008](0008-hj60-read-oem-sender-ratiometric.md) | Read the OEM gauge sender ratiometrically | Proposed | hj60_coolant |
| [0009](0009-hj60-12v-power-front-end.md) | 12 V automotive power front end | Proposed | hj60_coolant |
