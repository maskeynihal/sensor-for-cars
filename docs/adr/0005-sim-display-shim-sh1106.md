# 0005. SH1106 → SSD1306 shim library for Wokwi

- Status: Accepted
- Date: 2026-11-01
- Projects: water_monitor

## Context

The real OLED is a 1.3" **SH1106** (Adafruit_SH110X, `Adafruit_SH1106G`). Wokwi has SSD1306 and SH1107 parts but
**no SH1106**. Both are 128x64 I2C at 0x3C with an almost identical GFX API. Only the `begin()` signature and the
colour constant names differ. The sim .ino must stay byte-identical to firmware (ADR 0003).

A header next to the sketch does **not** work. arduino-cli resolves `#include <Adafruit_SH110X.h>` to the real
library (tested).

## Decision

`projects/water_monitor/emulator/wokwi/shims/Adafruit_SH110X_SimShim/` is a tiny library with its own
`library.properties`. Its `Adafruit_SH110X.h` defines `Adafruit_SH1106G` as a subclass of `Adafruit_SSD1306` and maps
`SH110X_*` colours. `project.env` lists it in `SIM_SHIMS`. `scripts/build.sh` passes it with `--library`, which takes
priority over `--libraries ./libraries`. Verified: the build log shows `Adafruit_SH110X_SimShim` + `Adafruit SSD1306`.

## Consequences

- The sim shows the same pixels, but on an SSD1306 controller. SH1106-specific quirks (the 132-column RAM offset)
  are not exercised.
- This pattern is the template for any future part Wokwi lacks: a shim library in `emulator/wokwi/shims/` plus an ADR.
