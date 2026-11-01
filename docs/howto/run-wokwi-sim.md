# Run the Wokwi simulation

Needs `WOKWI_CLI_TOKEN` (see [setup-toolchain](setup-toolchain.md)).

```sh
make sim P=water_monitor                          # all scenarios in emulator/wokwi/scenarios/
make sim P=water_monitor S=scenarios/smoke.yaml   # one scenario
make sim-live P=water_monitor                     # 60 s free run, serial in terminal
```

Outputs go to `build/<p>/wokwi/`: `<scenario>.serial.log` and `<scenario>.oled.png` (screenshot at 8 s).

## Files

| File | Purpose |
|---|---|
| `emulator/wokwi/diagram.json` | Circuit: board, parts, wires. Lint with `wokwi-cli lint projects/<p>/emulator/wokwi` |
| `emulator/wokwi/wokwi.toml` | Points at `build/<p>/sim/<p>_sim.ino.hex/.elf` |
| `emulator/wokwi/scenarios/*.yaml` | Automation steps: `wait-serial`, `set-control`, `expect-pin`, `delay`, `take-screenshot` |
| `emulator/wokwi/shims/` | Sim-only libraries for parts Wokwi lacks |

## Interactive (GUI)

Install the **Wokwi for VS Code** extension. Run `make build-sim P=<p>`, open `projects/<p>/emulator/wokwi/` as the
workspace folder (or the `diagram.json`), then run "Wokwi: Start Simulator". In the GUI you can drag the DS18B20 and
DHT22 sliders to test thresholds by hand.

## Part controls usable in scenarios

| Part | `set-control` name |
|---|---|
| `wokwi-ds18b20` | `temperature` (°C, -55..125) |
| `wokwi-pushbutton` | `pressed` (0/1) |
| `wokwi-potentiometer` | `position` |

The DHT22 has no documented scenario control. Set it with `attrs` in diagram.json instead.
