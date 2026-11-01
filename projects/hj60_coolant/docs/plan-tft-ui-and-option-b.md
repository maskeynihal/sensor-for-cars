# Plan: FriedRust TFT dashboard and option B (direct sender measurement)

- Date: 2026-11-01
- Status: in progress
- Design source: the owner's mockup (6 screens, orange and cream on black). Art crops are in `docs/img/`.

## Decisions (owner answers, 2026-11-01)

| Question | Answer |
|---|---|
| Display | ILI9488 3.5" 480x320 SPI TFT ([ADR 0010](../../../docs/adr/0010-hj60-ili9488-tft-and-sim-shim.md)) |
| Data that we cannot measure yet | Water and battery are real. RPM and speed show **DEMO** values. |
| Turbo | No turbo (stock 2H). Remove the BOOST and EGT tiles. |
| Option A and option B | One setting at the top of the sketch (`senderMode`). Both modes have tests. |

## Screens

The mockup has 6 panels. Without BOOST and EGT, they become 5 screens:

| # | Mockup panel | Screen | Content |
|---|---|---|---|
| 1 | Boot screen | BOOT | FRIEDRUST title art (bitmap from the mockup), 1.5 s |
| 2 | Boot screen (progress) | CHECK | Truck art, progress bar, "CHECKING SYSTEMS...", then the sender, battery and display results |
| 3 | Main gauge (driving) and 4 (idle) | MAIN | RPM (big) + bar 0–5 ×1000, SPD, then WATER and BATT tiles with bars |
| 5 | Grid | GRID | 4 tiles: RPM, SPEED, WATER TEMP, BATTERY |
| 6 | Big water | WATER | Big water temperature with bar, RPM, SPEED, BATT |

Rules for the screens:

- A button (GPIO33) selects the next screen: MAIN → GRID → WATER → MAIN.
- WARN and ALARM change to the WATER screen automatically. The frame is red. ALARM flashes the frame.
- A fault shows a banner with the fault name.
- RPM and speed have a small "DEMO" label until real sensors exist.
- The screen redraws only the values that changed. The ILI9488 uses 3 bytes for each pixel over SPI, so a full
  redraw takes about 0.1–0.15 s.

## Option B in the firmware

- `senderMode` = `SENDER_MODE_A` (passive taps) or `SENDER_MODE_B` (direct). Change it at the top of the sketch.
- Option B pins: GPIO26 switches the excitation on. GPIO34 measures Vtop through a 10k/10k divider. GPIO35 measures
  the sender voltage Vs through a 10k series resistor.
- The ESP32 switches the excitation on for each measurement, waits for the voltage to become stable, and reads
  Vtop and Vs. Then it switches the excitation off.
- `Rs = Rref × Vs / (Vtop − Vs)`, with Rref = 100 Ω.
- Faults are the same as in option A: open sender, shorted sender, and no excitation (Vtop too low).
- **Warning:** with option B, the dash gauge does not operate. The ESP32 display is then the only temperature gauge.

## Pins (ESP32 DevKit V1)

| Function | GPIO | Note |
|---|---|---|
| TFT SCK / MOSI | 18 / 23 | VSPI. MISO not connected. |
| TFT CS / DC / RST | 5 / 16 / 17 | |
| TFT backlight | 21 | PWM dimming |
| Buzzer (via NPN) | 25 | |
| Excitation switch (option B) | 26 | High = on |
| Battery voltage (68k/10k) | 32 | ADC1 |
| Feed (A) / Vtop (B) | 34 | ADC1 |
| Sender (A) / Vs (B) | 35 | ADC1 |
| Screen button | 33 | To ground, internal pull-up |

## Work steps

1. Plan and ADR 0010. **(this file)**
2. Mocks: `DIYables_TFT_SPI.h` for native tests. The mock keeps the text at each cursor position, so the tests can
   read the current screen.
3. Sim shim: `DIYables_TFT_SPI.h` that maps `DIYables_ILI9488_SPI` to an ILI9341 (320x240) for Wokwi.
4. Art: convert the two boot images to 16-colour packed bitmaps (`boot_art.h`). The firmware scales them to the
   screen size.
5. Firmware (sim copy first):
   - sender measurement for modes A and B
   - battery voltage
   - demo RPM and speed
   - screens and the button
   - alarms, faults and the buzzer (same as today)
6. Native tests: mode B conversion, faults, excitation timing, battery, screen text, button, alarm screen change.
   Keep the option A tests.
7. Wokwi: ILI9341, button, potentiometers for Vtop, Vs and battery. Scenario: boot → OK → ALARM → open sender.
8. `make test`, `make sim`, `make promote`. Report the results.

## Not in this work

- Real RPM and speed sensors (alternator W terminal and speedometer cable sensor, or GPS).
- Option C (gauge driver and relay).
- Logging and limits that the driver can change.
