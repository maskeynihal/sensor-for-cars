---
description: Ask what we're building and on which processor, then scaffold projects/<name>/
argument-hint: "[rough idea]"
---
The user wants to start a new hardware project. Rough idea: $ARGUMENTS

1. Read `AGENTS.md` § "Starting a new project" and `docs/howto/new-project.md`.
2. Ask everything in ONE AskUserQuestion batch (max 4 questions; fold the rest into "Other"):
   - What it does: inputs, outputs, and the one-sentence purpose.
   - Processor/board. Offer the boards already used in `projects/*/project.env` first, then ESP32 DevKit, Pico, Nano.
   - Parts list (sensors, display, actuators).
   - Power and where it lives in the car.
   Then propose a snake_case name and a sim board (closest board Wokwi supports), and confirm both.
3. Resolve FQBNs: `arduino-cli board listall <word>`. Install a missing core (`arduino-cli core install ...`).
   Third-party cores (ESP32, RP2040) need `arduino-cli config add board_manager.additional_urls <url>` first.
4. Check that each part exists in Wokwi (https://docs.wokwi.com/getting-started/supported-hardware). If a part is
   missing, plan a shim under `emulator/wokwi/shims/` like water_monitor's SH1106 one.
5. Scaffold: `./scripts/new-project.sh <name> <fqbn> <sim_fqbn> "<board>" "<purpose>" <baud>`
6. Write `docs/ideas/<YYYY-MM-DD>-<name>.md` from `docs/ideas/_template.md`. Write an ADR for every new board or part.
7. Fill in `projects/<name>/docs/README.md` (BOM + wiring table) and `emulator/wokwi/diagram.json` with the parts.
   Run `wokwi-cli lint projects/<name>/emulator/wokwi`.
8. Run `make test P=<name>` and `make build P=<name>`. Report the pass counts, flash %, and RAM %.
