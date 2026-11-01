# Start a new project

In Claude Code: `/new-project <rough idea>`. The agent asks the questions below, then runs the scaffolder.

## Questions to answer first

1. Name (snake_case)
2. Purpose, inputs, outputs
3. Real board + FQBN: `arduino-cli board listall esp32`, `... uno`, etc.
4. Sim board: the closest Wokwi-supported board
5. Parts: check that each exists in Wokwi (https://docs.wokwi.com/getting-started/supported-hardware)
6. Power and mounting in the car

## Scaffold

```sh
./scripts/new-project.sh <name> <fqbn> <sim_fqbn> "<Board label>" "<purpose>" <baud>
# e.g.
./scripts/new-project.sh oil_temp arduino:renesas_uno:unor4wifi arduino:avr:uno \
  "Arduino UNO R4 WiFi" "Oil temperature gauge" 9600
```

| Real board | FQBN | Suggested sim FQBN |
|---|---|---|
| UNO R4 WiFi | `arduino:renesas_uno:unor4wifi` | `arduino:avr:uno` |
| UNO R4 Minima | `arduino:renesas_uno:minima` | `arduino:avr:uno` |
| Uno R3 | `arduino:avr:uno` | same |
| Nano | `arduino:avr:nano` | same |
| Mega 2560 | `arduino:avr:mega` | same |
| ESP32 DevKit | `esp32:esp32:esp32` | same |
| ESP32-S3 | `esp32:esp32:esp32s3` | same |
| Pico (Philhower core) | `rp2040:rp2040:rpipico` | same |

The script copies `templates/project/` (a heartbeat sketch, native tests, a Wokwi diagram, and a smoke
scenario), installs missing cores, then runs the tests and both builds.

## After scaffolding

- [ ] `docs/ideas/<date>-<name>.md`
- [ ] ADR for every new board or part (and for each shim)
- [ ] `projects/<name>/docs/README.md`: BOM, wiring table, power
- [ ] `emulator/wokwi/diagram.json`: add parts, then `wokwi-cli lint`
- [ ] Native tests for each behaviour, and a Wokwi scenario for each behaviour
- [ ] Root README project table
