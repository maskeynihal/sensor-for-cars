# Utility sketches

Small, single-file bring-up tools. They're uploadable as-is and have no emulator copy.

| Sketch | Use |
|---|---|
| `blink_led/` | First smoke test: blinks D8 |
| `i2c_address_finder/` | Scans I2C and prints addresses at 115200 (found the OLED at 0x3C) |
| `esp32_gauge_capture/` | ESP32: logs the HJ60 temp gauge feed + sender at 1 kHz for 30 s as CSV (hj60_coolant step M2) |

```sh
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi --libraries ./libraries projects/_utilities/i2c_address_finder
arduino-cli upload  --fqbn arduino:renesas_uno:unor4wifi -p /dev/cu.usbmodemXXXX projects/_utilities/i2c_address_finder
```

ESP32-S3 (DevKitC-1, hj60_coolant pins):

```sh
arduino-cli compile --fqbn esp32:esp32:esp32s3 projects/_utilities/esp32_gauge_capture
arduino-cli upload  --fqbn esp32:esp32:esp32s3 -p /dev/cu.usbserial-XXXX projects/_utilities/esp32_gauge_capture
```
