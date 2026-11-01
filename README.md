# sensor-for-cars

Microcontroller projects for the car (sensors, displays, alarms), built so a coding agent can develop and
test them on a Mac with emulators before anything is flashed to a real board.

## Projects

| Project | Board | What |
|---|---|---|
| [water_monitor](projects/water_monitor/) | Arduino UNO R4 WiFi | **LC78 OFFROAD**: DS18B20 water probe, DHT22 air, animated SH1106 OLED, buzzer alarm |
| [hj60_coolant](projects/hj60_coolant/) | ESP32 DevKit V1 | HJ60 coolant temperature from the factory sender (mode A taps or mode B direct), FriedRust 3.5" ILI9488 TFT dashboard, buzzer |
| [_utilities](projects/_utilities/) | any | blink, I2C scanner |
| [_archive](projects/_archive/) | | superseded sketches |

## Layout

```
projects/<p>/firmware/<p>/<p>.ino        uploadable sketch (real board)
projects/<p>/emulator/wokwi/<p>_sim/     sim copy, where changes are made + Wokwi circuit/scenarios
projects/<p>/emulator/native/            host tests (run against both copies)
projects/<p>/docs/                       wiring, BOM, runtime architecture
libraries/                               vendored Arduino libraries (don't edit)
tools/native/                            Arduino mocks + test runner
templates/project/                       new-project skeleton
docs/{ideas,adr,howto,architecture}/     repo-wide docs
```

Why it's split this way: [ADR 0003](docs/adr/0003-projects-layout-firmware-vs-emulator.md),
[ADR 0004](docs/adr/0004-two-tier-emulation.md).

## Quick start

```sh
make doctor                    # arduino-cli, wokwi-cli, clang++, cores, token
make test P=water_monitor      # 13 host tests, sim copy + firmware
make build P=water_monitor     # compile for the Wokwi board (AVR Uno) + the real board (UNO R4 WiFi)
make sim P=water_monitor       # Wokwi scenarios (needs WOKWI_CLI_TOKEN)
make promote P=water_monitor   # tested sim copy -> firmware (gated, auto-rollback)
make upload P=water_monitor PORT=/dev/cu.usbmodemXXXX
make new P=<name> FQBN=... SIM_FQBN=...   # or /new-project in Claude Code
make help
```

## Working with an agent

- Agent rules: [AGENTS.md](AGENTS.md). Claude Code also reads [CLAUDE.md](CLAUDE.md) and `.claude/commands/`, and
  Kiro reads `.kiro/steering/`.
- Workflow diagram: [docs/architecture/dev-workflow.md](docs/architecture/dev-workflow.md)
- How-tos: [docs/howto/](docs/howto/). Decisions: [docs/adr/](docs/adr/). Ideas: [docs/ideas/](docs/ideas/)

The loop: the agent edits the **sim copy**, then runs host tests and Wokwi, then promotes to **firmware**. You
upload.
