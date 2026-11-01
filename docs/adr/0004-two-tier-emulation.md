# 0004. Two-tier emulation: native host tests + Wokwi

- Status: Accepted
- Date: 2026-11-01
- Projects: all

## Context

Facts checked 2026-11-01:
- water_monitor runs on an **Arduino UNO R4 WiFi (Renesas RA4M1, Cortex-M4)**.
- **Wokwi does not simulate the RA4M1.** It supports AVR (Uno/Nano/Mega), ESP32 family, RP2040/RP2350, and a few
  STM32 ([supported hardware](https://docs.wokwi.com/getting-started/supported-hardware)). simavr is AVR-only.
- Wokwi CI (`wokwi-cli`) needs `WOKWI_CLI_TOKEN`. The free tier gives **50 sim-minutes per rolling 30 days** and
  5 min per run.
- Agents need a test they can *always* run, offline and free.

## Options

1. **Wokwi only**: realistic, but it costs quota on every run, needs a token, and can't run the real board's core.
2. **Native only**: free and fast, but the library code is mocked and there's no visual.
3. **Both.**

## Decision

Option 3.
- **Tier 1, native** (`make test`): `tools/native/mocks` provides a fake Arduino core plus mocks for the libraries in
  use. The **unmodified .ino** is compiled with clang++ and driven by `test_*.cpp` with fake time. It runs on both the
  sim copy and the firmware. This is the agent's default check.
- **Tier 2, Wokwi** (`make sim`): the sim copy is compiled for the closest Wokwi board (`SIM_FQBN`) and run against
  `diagram.json` with YAML scenarios. It is used before promote, and whenever the change touches real library
  behaviour, I2C, or display output.

## Consequences

- The mocks must behave like the real parts (e.g. the DS18B20 latches its reading on `requestTemperatures()`).
  Wrong mocks give false confidence. Wokwi is the cross-check.
- Board-specific behaviour (R4 WiFi radio, LED matrix, 32 KB RAM) can't be emulated. It is verified on hardware
  after upload.
- A sim board with less RAM can fail where the real board wouldn't. Watch `make build-sim` RAM %.
