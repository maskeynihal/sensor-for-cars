# Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `error: missing project name` | Pass `P=<name>`. `make projects` lists them. |
| Native build: `'foo' was not declared` | A function is used before it's defined. Move it up or add a prototype (the native build doesn't auto-generate them). |
| Native build: missing header `X.h` | No mock yet. Add `tools/native/mocks/X.h`. |
| Native test `timed out after 5 s` | The sketch hit `for(;;);` (e.g. `display.begin` failed) or a `while` with no `delay()`. |
| Sim build uses the real `Adafruit_SH110X` | The shim isn't in `SIM_SHIMS` in project.env. `arduino-cli compile -v` shows "Alternatives for ...". |
| Sim build RAM > 85% | Use `F()` strings, or move the sim to `arduino:avr:mega` (ADR 0006). |
| `Missing WOKWI_CLI_TOKEN` | See setup-toolchain. |
| Wokwi: "SH1106 allocation failed" on serial | The 1 KB display buffer malloc failed on the AVR Uno. Free RAM or use a Mega. |
| Wokwi timeout exit code 42 | A `wait-serial` text never appeared. Check `build/<p>/wokwi/*.serial.log`. |
| Upload: no port | Data cable, double-tap RESET, `arduino-cli board list`. |
