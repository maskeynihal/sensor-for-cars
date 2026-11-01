# 0006. water_monitor: real board UNO R4 WiFi, sim board AVR Uno

- Status: Accepted
- Date: 2026-11-01
- Projects: water_monitor

## Context

- Real: UNO R4 WiFi, 256 KB flash / 32 KB RAM. Firmware uses 32% flash and 29% RAM (static).
- Wokwi can't run the R4 (ADR 0004). The closest board with the same pinout (D2/D3/D4, A4/A5 I2C) is the
  **AVR Uno**: 32 KB flash / **2 KB RAM**.
- Sim build: 74% flash, 751 B static RAM. The SSD1306 library also `malloc`s a 1 KB frame buffer in `begin()`,
  which leaves roughly 270 B of stack.
- `DHT::read()` keeps `uint32_t cycles[80]` (320 B) on the stack, which is **more than that headroom**. On the
  Uno the stack may run into the heap (the OLED buffer). Because the frame is redrawn every loop, this could be
  harmless or could cause glitches or resets. The first Wokwi run settles it.

## Options

1. **Uno**: identical pin numbers and wiring, so diagram.json mirrors the real breadboard. RAM is tight.
2. **Mega 2560**: 8 KB RAM and plenty of headroom, but I2C moves to pins 20/21, so the wiring diagram differs.

## Decision

Uno, because the wiring matches. If the sim resets, shows garbage, misses DHT readings, or `display.begin()` fails ("SH1106 allocation
failed"), switch `SIM_FQBN` to `arduino:avr:mega` and the diagram board to `wokwi-arduino-mega`, and supersede this ADR.

## Consequences

- New strings in the sketch should use `F("...")`. Current `Serial.print("Water: ")`-style literals cost AVR RAM.
- RAM headroom on the sim isn't a concern for the R4 itself.
