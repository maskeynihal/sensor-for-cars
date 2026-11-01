# hj60_coolant -- hardware

Why the circuit looks like this: [gauge-tap-report.md](gauge-tap-report.md). Keep this file in step with
`emulator/wokwi/diagram.json`.

## Bill of materials

Common parts:

| Qty | Part | Notes |
|---|---|---|
| 1 | ESP32-S3 DevKitC-1 N16R8 (16 MB flash, 8 MB PSRAM) | 5 V into 5V/VIN. GPIO7–37 are flash/PSRAM: do not use them. |
| 1 | ILI9488 3.5" 480x320 SPI TFT | 3.3 V logic. MISO not connected. Check the LED pin: if the module has no backlight transistor, drive it through an NPN or MOSFET (60–100 mA). |
| 1 | Momentary push button | GPIO15 to ground. Internal pull-up. |
| 1 | Active buzzer 5 V + NPN (2N2222 / S8050) + 1 kΩ base resistor + 1N4148 across the buzzer | Buzzer driver |
| 1 | Car USB adapter (12 V socket, 2 A) + USB cable | Powers the ESP32 and the TFT through the board's UART USB port |
| (optional) | 68 kΩ + 10 kΩ + 100 nF | Battery voltage divider to GPIO6. Only with `batterySense = true`. |
| (optional) | 12 V → 5 V automotive front end | Only if you hard-wire to 12 V later. See [ADR 0009](../../../docs/adr/0009-hj60-12v-power-front-end.md). |
| - | Locking connectors, soldered perfboard, strain relief | No breadboard or Dupont in the truck |

Sender circuit, **mode B** (direct, default in the sketch). Resistors only, no diodes or transistors:

| Qty | Part | Notes |
|---|---|---|
| 1 | Rref 220 Ω, 1% or better | GPIO7 → Rref → YG wire. GPIO7 powers the sender while measuring (~13 mA, 2 ms in each 100 ms). |
| 2 | 47 kΩ, 1% or better | Top of each divider |
| 2 | 100 kΩ, 1% or better | Bottom of each divider. Vtop (GPIO7 side of Rref) → GPIO4, Vs → GPIO5 |
| 2 | 10–100 nF (optional) | GPIO4 and GPIO5 to GND |
| (later) | Schottky diode 1N5819 in series with GPIO7, BAT54S clamps on GPIO4/5 | Protects GPIO7 if the YG wire touches 12 V. Recommended before you drive with it. |

Sender circuit, **mode A** (passive taps):

| Qty | Part | Notes |
|---|---|---|
| 2 | 68 kΩ + 10 kΩ 0.1% + 100 nF + BAT54S | Gauge E → GPIO4, gauge F → GPIO5 |

Full schematic and wire list: [html/wiring-diagram.html](html/wiring-diagram.html) ([PNG](img/wiring-diagram.png)).

## Pins

| Function | GPIO | Note |
|---|---|---|
| TFT SCK / MOSI | 12 / 11 | FSPI default pins |
| TFT CS / DC / RST | 10 / 9 / 8 | |
| TFT backlight | 18 | PWM |
| Buzzer (via NPN) | 17 | |
| Sender power (mode B) | 7 | Drives Rref directly. HIGH = on, about 2 ms in each 100 ms |
| Battery (68k/10k) | 6 | ADC1 |
| Mode A: gauge E / Mode B: Vtop | 4 | ADC1 |
| Mode A: gauge F / Mode B: Vs | 5 | ADC1 |
| Screen button | 15 | To ground |

## Wiring

Mode B (YG wire disconnected from the gauge):

```
 GPIO7 ──┬──[ Rref 220 Ω ]──┬── VS ── YG wire ── sender ── ⏚ engine block
         │                  │
       47k                47k
         ├──► GPIO4         ├──► GPIO5
       100k               100k
         │                  │
        GND                GND        ESP32 GND ── truck body ground
```

Mode A (dash gauge stays connected):

| From | To | Notes |
|---|---|---|
| Temp gauge terminal **E** (after the regulator) | 68k → GPIO4, 10k + 100 nF to GND | **Not cluster connector pin A**. E–F must read ~25 Ω (step M0) |
| Temp gauge terminal **F** / sender wire (YG) | 68k → GPIO5, 10k + 100 nF to GND | The gauge stays connected |

Both modes:

| From | To | Notes |
|---|---|---|
| Cluster ground | ESP32 GND | Same ground the ADC inputs reference |
| Ignition 12 V (after the fuse and the reverse-protection diode) | 68k → GPIO6, 10k + 100 nF to GND | Battery voltage (reads about 0.3 V low behind a Schottky) |
| TFT VCC / GND | 3.3 V or 5 V (check the module) / GND | |
| Button | GPIO15 and GND | |
| Buzzer (via Q1 collector) | GPIO17 → 1 kΩ → Q1 base | Buzzer + to 5 V |
| Front end 5 V out | ESP32 5V/VIN | Never 12 V straight in |

Rules:

- Mode A taps are high resistance only. Nothing below ~100 kΩ on E or F.
- No ohmmeter on the sender while it is connected or powered.
- Do not mix up the gauge sender (single terminal, in the head) and the 2-pin glow-plug-control sensor.
- Mode B: check the backfeed fault on the bench. Connect the YG wire to 12 V through a 1 kΩ resistor. The screen must
  show "WIRE HAS VOLTAGE".

## Power and ground

- **USB power** (owner's choice, 2026-11-01): a car USB adapter in the 12 V socket powers the ESP32 and the TFT
  through the board's UART USB port. No 12 V wiring to the board.
- **Ground:** connect ESP32 GND to the engine block near the sender (or the sender mount). The sender returns its
  current through the engine block, so the circuit is not complete without this wire. A short wire to the block gives
  the smallest error. The USB adapter's ground is usually the car ground too, and that is fine.
- **Battery tile:** shows "USB" because `batterySense = false`. To show the battery voltage, wire the 68k/10k divider
  from a fused 12 V point to GPIO6, set `batterySense = true`, and flash again.
- The truck is 12 V, one battery: 12.9 V engine off, 14.0 V running. A cranking dip can reset the USB adapter. The
  ESP32 then restarts in about a second.

## Sim vs real differences

| Real hardware | Emulator | Why |
|---|---|---|
| ESP32-S3 DevKitC-1 N16R8 | ESP32-S3 DevKitC-1 | Same chip |
| ILI9488 480x320 | ILI9341 320x240 via `emulator/wokwi/shims` | Wokwi has no ILI9488 ([ADR 0010](../../../docs/adr/0010-hj60-ili9488-tft-and-sim-shim.md)). The UI scales. |
| Mode B sender circuit | Custom chip `chips/hj60-sender.chip.c` | Wokwi pots ignore VCC, so they cannot model the switched excitation ([ADR 0011](../../../docs/adr/0011-hj60-wokwi-sender-chip.md)) |
| Battery divider | Potentiometer on GPIO6 | Wokwi does not simulate resistor networks |
| ESP32 ADC (calibrated) | Wokwi ADC model with ~+0.14 V offset | The sim reads 10–20 °C low at the hot end. The scenario checks states only. |
| Mode A pulsing regulator, contact bounce | Native tests only | |
