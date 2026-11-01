# hj60_coolant -- hardware

Why the circuit looks like this: [gauge-tap-report.md](gauge-tap-report.md). Keep this file in step with
`emulator/wokwi/diagram.json`.

## Bill of materials

| Qty | Part | Notes |
|---|---|---|
| 1 | ESP32 DevKit V1 (WROOM-32) | 5 V into VIN/5V |
| 1 | 1.3" SH1106 OLED 128x64 I2C | 0x3C, 3.3 V |
| 1 | Active buzzer 5 V | Driven through Q1 |
| 1 | NPN transistor (2N2222 / S8050) + 1 kΩ base resistor + 1N4148 across the buzzer | Buzzer driver |
| 2 | 120 kΩ 0.1% | Divider top, one per tap |
| 2 | 10 kΩ 0.1% | Divider bottom |
| 2 | 100 nF ceramic | Anti-alias, ADC pin to GND |
| 2 | BAT54S | Clamp each ADC pin to 3V3 / GND |
| 1 | 24 V → 5 V automotive front end | See [ADR 0009](../../../docs/adr/0009-hj60-24v-power-front-end.md): 1 A fuse, reverse protection, TVS (standoff > 32 V), buck rated ≥ 60 V in |
| - | Locking connectors, soldered perfboard, strain relief | No breadboard or Dupont in the truck |

## Wiring

| From | To | Notes |
|---|---|---|
| Temp gauge terminal **E** (regulated feed, after the regulator) | 120k → GPIO34, 10k + 100 nF to GND | Feed tap. **Not cluster connector pin A** (regulator input). E–F must read ~25 Ω (report step M0) |
| Temp gauge terminal **F** / sender wire (YG) | 120k → GPIO35, 10k + 100 nF to GND | Sender tap. The gauge stays connected |
| Cluster ground | ESP32 GND | Same ground the taps reference |
| OLED SDA / SCL | GPIO21 / GPIO22 | 3V3 / GND |
| Buzzer (via Q1 collector) | GPIO25 → 1 kΩ → Q1 base | Buzzer + to 5 V |
| Front end 5 V out | ESP32 5V/VIN | Never 24 V |

Rules:

- The taps are high impedance only. Nothing below ~100 kΩ on E or F.
- No ohmmeter on the sender while it's connected or powered.
- Don't confuse the gauge sender (single terminal, in the head) with the 2-pin glow-plug-control sensor.

## Power

24 V truck (two batteries in series, ~28–29 V charging). Ignition-switched, through the front end in ADR 0009.

## Sim vs real differences

| Real hardware | Emulator | Why |
|---|---|---|
| ESP32 DevKit V1 | ESP32 DevKit C v4 | Same chip (WROOM-32) |
| SH1106 | SSD1306 via `emulator/wokwi/shims` | Wokwi has no SH1106 ([ADR 0005](../../../docs/adr/0005-sim-display-shim-sh1106.md)) |
| Gauge feed + sender through 13:1 dividers | Two potentiometers straight onto GPIO34/35 | Wokwi doesn't simulate resistor networks or the pulsing regulator |
| Pulsing bimetal regulator, contact bounce | Native tests only (`Truck` model in `test_hj60_coolant.cpp`) | |
| ESP32 ADC with eFuse calibration | Wokwi ADC reads about 3.47 V × position + 0.137 V | The scenario picks pot positions from the measured mapping |
