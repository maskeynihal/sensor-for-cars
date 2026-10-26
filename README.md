# sensor-for-cars

Arduino UNO R4 WiFi water + air monitor with an animated 1.3" SH1106 OLED
("LC78 OFFROAD", built by FriedRust).

## Folder map

All sketches live under `sketches/`.

| Folder | What it is | Keep? |
|---|---|---|
| `sketches/water_monitor/` | **Final build** -- DS18B20 + DHT22 + OLED + buzzer | yes |
| `sketches/i2c_address_finder/` | I2C scanner, finds the OLED address (0x3C) | utility |
| `sketches/blink_led/` | Minimal blink on D8, first board smoke-test | utility |
| `sketches/lc78_oled_animation/` | Early OLED-only LC78 animation, **no sensors** (superseded by `water_monitor/`) | history |
| `libraries/` | Third-party Arduino libraries (do not edit) | yes |

> `sketches/lc78_oled_animation/` is an earlier experiment kept for reference.
> Only `sketches/water_monitor/` is the shipping sketch.

## Quick start

1. Open `sketches/water_monitor/water_monitor.ino` in the Arduino IDE.
2. Install libraries (see docs): Adafruit SH110X, Adafruit GFX, OneWire,
   DallasTemperature, DHT sensor library (+ Adafruit Unified Sensor).
3. Board: **Arduino UNO R4 WiFi**. Wire per [`docs/`](docs/README.md).
4. Upload. Open Serial Monitor at **9600 baud**.

## Build from the command line (arduino-cli)

The libraries this project needs are already vendored in `libraries/`, so you
do not have to install anything by name — just point arduino-cli at that folder
with `--libraries`.

```sh
# one-time: install the board core
arduino-cli core update-index
arduino-cli core install arduino:renesas_uno

# compile using the repo's own libraries/ folder
arduino-cli compile \
  --fqbn arduino:renesas_uno:unor4wifi \
  --libraries ./libraries \
  sketches/water_monitor

# find your board's port (Port column; e.g. /dev/cu.usbmodemXXXX)
arduino-cli board list

# upload (replace the port with the one from 'arduino-cli board list')
arduino-cli upload \
  --fqbn arduino:renesas_uno:unor4wifi \
  --libraries ./libraries \
  -p /dev/cu.usbmodemXXXX \
  sketches/water_monitor
```

Prefer the registry instead of the vendored copies? Install them by name once:

```sh
arduino-cli lib install "Adafruit SH110X" "Adafruit GFX Library" \
  "OneWire" "DallasTemperature" "DHT sensor library"
```

Then compile without the `--libraries` flag.
