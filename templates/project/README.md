# __NAME__

__PURPOSE__

| | |
|---|---|
| Board | __BOARD__ (`__FQBN__`) |
| Emulator board | __SIM_BOARD__ (`__SIM_FQBN__`) |
| Serial | __BAUD__ baud |
| Status | scaffolded __DATE__ |

## Folders

| Path | What | Rule |
|---|---|---|
| `firmware/__NAME__/` | Uploadable sketch for the real board | Only changed via `make promote`, or by hand when the change cannot be simulated |
| `emulator/wokwi/__NAME___sim/` | Sim copy, where changes are made and tested | Same code as firmware once promoted |
| `emulator/wokwi/diagram.json` | Wokwi circuit | Keep in step with `docs/README.md` wiring |
| `emulator/wokwi/scenarios/` | Wokwi automation scenarios | One per behaviour |
| `emulator/native/` | Host tests (`test_*.cpp`) | Run against both copies |
| `docs/` | Wiring, BOM, architecture for this project | |

## Commands

```sh
make test     P=__NAME__     # host tests, no hardware
make sim      P=__NAME__     # Wokwi scenarios (needs WOKWI_CLI_TOKEN)
make promote  P=__NAME__     # sim copy -> firmware
make upload   P=__NAME__ PORT=/dev/cu.usbmodemXXXX
```

## Docs

- [Hardware + wiring](docs/README.md)
- [Software architecture](docs/architecture.md)
