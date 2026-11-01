# hj60_coolant

Engine coolant temperature for a Toyota Land Cruiser HJ60 (2H, 24 V), read from the factory gauge sender without
disturbing the dash gauge. OLED readout and buzzer alarm.

| | |
|---|---|
| Board | ESP32 DevKit V1 (`esp32:esp32:esp32`), [ADR 0007](../../docs/adr/0007-hj60-coolant-on-esp32-devkit.md) |
| Emulator board | ESP32 DevKit C v4 (`esp32:esp32:esp32`) |
| Serial | 115200 baud |
| Parts | Gauge taps E/F via 13:1 dividers on GPIO34/35, SH1106 OLED (21/22), active buzzer via NPN (GPIO25) |
| Status | Native tests and Wokwi pass. Not on the truck yet: needs measurements M0–M6 ([report](docs/gauge-tap-report.md#6-plan)). **Don't trust WARN/ALARM until the M4 sender calibration** (two-point curve, ±10 °C near 100 °C) |
| Idea | [docs/ideas/2026-11-01-hj60-coolant.md](../../docs/ideas/2026-11-01-hj60-coolant.md) |

## Folders

| Path | What | Rule |
|---|---|---|
| `firmware/hj60_coolant/` | Uploadable sketch for the real board | Only changed via `make promote`, or by hand when the change cannot be simulated |
| `emulator/wokwi/hj60_coolant_sim/` | Sim copy, where changes are made and tested | Same code as firmware once promoted |
| `emulator/wokwi/diagram.json` | Wokwi circuit | Keep in step with `docs/README.md` wiring |
| `emulator/wokwi/scenarios/` | Wokwi automation scenarios | One per behaviour |
| `emulator/native/` | Host tests (`test_*.cpp`) | Run against both copies |
| `docs/` | Wiring, BOM, architecture for this project | |

## Commands

```sh
make test     P=hj60_coolant     # host tests, no hardware
make sim      P=hj60_coolant     # Wokwi scenarios (needs WOKWI_CLI_TOKEN)
make promote  P=hj60_coolant     # sim copy -> firmware
make upload   P=hj60_coolant PORT=/dev/cu.usbserial-XXXX
```

## Docs

- [Report: what the multimeter showed, what went wrong, the plan](docs/gauge-tap-report.md)
- [Hardware + wiring](docs/README.md)
- [Software architecture](docs/architecture.md)
