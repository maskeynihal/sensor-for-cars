# water_monitor: LC78 OFFROAD water + air monitor

Water temperature probe plus air temperature and humidity, shown on an animated 1.3" OLED (an LC78 driving over
terrain). A buzzer beeps when the water is warm and sounds continuously when it's hot.

| | |
|---|---|
| Board | Arduino UNO R4 WiFi (`arduino:renesas_uno:unor4wifi`) |
| Emulator board | Arduino Uno AVR (`arduino:avr:uno`). Wokwi can't run the R4 ([ADR 0006](../../docs/adr/0006-water-monitor-sim-on-avr-uno.md)) |
| Parts | DS18B20 (D2, 4.7k pull-up), DHT22 (D4), SH1106 OLED I2C 0x3C (A4/A5), active buzzer (D3) |
| Serial | 9600 baud, one line every ~2 s |
| Status | Working on hardware. Native tests 13/13. Wokwi scenario written, not run yet (no token) |
| Idea | [docs/ideas/2026-11-01-water-monitor.md](../../docs/ideas/2026-11-01-water-monitor.md) |

## Folders

| Path | What |
|---|---|
| `firmware/water_monitor/water_monitor.ino` | **Uploadable** sketch |
| `emulator/wokwi/water_monitor_sim/water_monitor_sim.ino` | Sim copy, where changes are made (identical to firmware when in sync) |
| `emulator/wokwi/shims/Adafruit_SH110X_SimShim/` | Maps SH1106 onto Wokwi's SSD1306 ([ADR 0005](../../docs/adr/0005-sim-display-shim-sh1106.md)) |
| `emulator/wokwi/diagram.json` | Simulated circuit (same pins as the real one) |
| `emulator/wokwi/scenarios/smoke.yaml` | Heats the probe 24 → 37 → 45 °C, checks the serial verdict and the buzzer pin |
| `emulator/native/test_water_monitor.cpp` | 13 host tests: thresholds, beep cadence, alarm, sensor failures, log format |
| `docs/architecture.md` | How the firmware runs: loop, timing, buzzer state machine |
| `docs/markdown/README.md` | Full hardware guide: BOM, pin map, circuit, bring-up, troubleshooting |
| `docs/markdown/sensor-explainer.md` | How the DS18B20 / DHT22 / OLED work |
| `docs/html/index.html` | Interactive version of the hardware guide + breadboard SVG |

## Commands

```sh
make test     P=water_monitor
make sim      P=water_monitor        # needs WOKWI_CLI_TOKEN
make promote  P=water_monitor
make upload   P=water_monitor PORT=/dev/cu.usbmodemXXXX
make monitor  P=water_monitor PORT=/dev/cu.usbmodemXXXX
```

## Build sizes (2026-11-01)

| Target | Flash | RAM (static) |
|---|---|---|
| UNO R4 WiFi | 85,960 B (32%) | 9,568 B (29%) |
| AVR Uno (sim) | 23,942 B (74%) | 751 B (36%), plus a 1 KB OLED buffer allocated at runtime |
