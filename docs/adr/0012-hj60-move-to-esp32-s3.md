# 0012. hj60_coolant: move to ESP32-S3 DevKitC-1 (N16R8)

- Status: Accepted
- Date: 2026-11-01
- Projects: hj60_coolant
- Supersedes: [0007](0007-hj60-coolant-on-esp32-devkit.md)

## Context

- The first upload failed: "This chip is ESP32-S3, not ESP32". The owner's board is an ESP32-S3.
- `esptool flash-id` reported: ESP32-S3 (QFN56) rev v0.2, embedded PSRAM 8 MB, flash 16 MB (quad). This is an
  N16R8-class DevKitC-1 board with a USB-UART bridge (`/dev/cu.usbserial-*`).
- On this module GPIO26–32 connect to the flash and GPIO33–37 to the PSRAM. ADC1 is GPIO1–10. The DevKit V1 pins
  (ADC 32/34/35, excitation 26, button 33) are not usable.
- Wokwi has the ESP32-S3 DevKitC-1 (`board-esp32-s3-devkitc-1`).

## Options

1. **Move the project to the ESP32-S3** with a new pin map.
2. **Keep the DevKit V1** and buy or plug in that board.

## Decision

Option 1 (the owner chose it). FQBN and sim FQBN `esp32:esp32:esp32s3` (default options: 4 MB flash map, no PSRAM,
USB CDC off, so `Serial` goes to the UART bridge).

| Function | DevKit V1 | ESP32-S3 |
|---|---|---|
| Feed (A) / Vtop (B) | 34 | 4 (ADC1_CH3) |
| Sender (A) / Vs (B) | 35 | 5 (ADC1_CH4) |
| Battery | 32 | 6 (ADC1_CH5) |
| Excitation (B) | 26 | 7 |
| Buzzer | 25 | 17 |
| Button | 33 | 15 |
| TFT SCK / MOSI | 18 / 23 | 12 / 11 (FSPI default) |
| TFT CS / DC / RST | 5 / 16 / 17 | 10 / 9 / 8 |
| TFT backlight | 21 | 18 |
| Option C PWM / relay (not built) | 27 / 13 | 16 / 21 |

The pins avoid the strapping pins (0, 3, 45, 46), USB (19, 20) and flash/PSRAM (26–37).

## Consequences

- Firmware: 502,029 bytes flash (38%), 25,936 bytes RAM (7%). The 39 native tests and the Wokwi scenario pass on the
  S3 build.
- The ADC on the S3 has a different curve from the classic ESP32. `ADC_OFFSET_MV` and step M5 still apply.
- The S3 can use 16 MB flash and 8 MB PSRAM later (logging, a full-screen frame buffer). That needs board options in
  the FQBN.
- Hardware docs, the explainer page and the capture utility now use the S3 pins.
