# LC78 OFFROAD -- Water + Air Monitor: full documentation

Firmware: [`sketches/water_monitor/water_monitor.ino`](../../sketches/water_monitor/water_monitor.ino)

Deep dive: [How the temperature sensors work — analog physics to °C/°F](sensor-explainer.md)

---

## 1. What it does

- Reads **water temperature** from a DS18B20 waterproof probe.
- Reads **air temperature + humidity** from a DHT22.
- Shows all three on a 1.3" SH1106 OLED, over a scrolling off-road truck animation.
- Prints a status line to Serial every 2 s.
- Sounds an **active buzzer** in three stages based on the water temperature.

---

## 2. Hardware (bill of materials)

| # | Part | Notes |
|---|---|---|
| 1 | Arduino UNO R4 WiFi | 5 V logic |
| 2 | SH1106 OLED 1.3" 128x64, I2C | address `0x3C` |
| 3 | DS18B20 waterproof probe | OneWire; needs a 4.7 kO pull-up |
| 4 | DHT22 (AM2302) breakout | temp + humidity; has its own pull-up |
| 5 | Active buzzer | fixed tone, on/off only |
| 6 | 4.7 kO resistor | DS18B20 data pull-up to 5 V |
| 7 | Breadboard + jumper wires | |

**Active buzzer** (confirmed by test): it makes a tone from plain `digitalWrite(HIGH)`.
It has one fixed pitch -- loudness cannot be changed in code. So "alarm" vs "warn" is
**steady vs pulsing**, not louder vs softer.

---

## 3. Pin map

| Part | Pin on part | UNO R4 pin |
|---|---|---|
| OLED | SDA | A4 |
| OLED | SCL | A5 |
| OLED | VCC / GND | 5V / GND |
| DS18B20 (water) | Red (VCC) | 5V |
| DS18B20 (water) | Black (GND) | GND |
| DS18B20 (water) | Yellow (DATA) | **D2** |
| DS18B20 (water) | 4.7 kO resistor | between DATA and 5V |
| DHT22 (air) | VCC / + | 5V |
| DHT22 (air) | DATA / OUT | **D4** |
| DHT22 (air) | GND / - | GND |
| Buzzer | + | **D3** |
| Buzzer | - | GND |

---

## 4. Circuit diagram

ASCII schematic. `[4.7k]` is the DS18B20 pull-up. Every VCC is 5 V, every GND common.

```
                         Arduino UNO R4 WiFi
                     +-------------------------+
                     |                         |
   OLED SDA  --------| A4                   5V |------+------+------+-----> 5V rail
   OLED SCL  --------| A5                      |      |      |      |
                     |                     GND |--+   |      |      |
                     |                         |  |   |      |      |
   water DATA -------| D2                      |  |   |      |      |
        |            |                         |  |   |      |      |
      [4.7k]         |                      D3 |--|---|------|------|---- buzzer (+)
        |            |                         |  |   |      |      |
        +--> 5V      |                      D4 |--|---|---+  |      |
                     |                         |  |   |   |  |      |
                     +-------------------------+  |   |   |  |      |
                                                  |   |   |  |      |
   GND rail <-------------------------------------+   |   |  |      |
      |   |   |                                       |   |  |      |
      |   |   +--- buzzer (-)                          |   |  |      |
      |   |                                            |   |  |      |
      |   +------- DHT22 GND          DHT22 VCC -------+---|--+      |   (to 5V rail)
      |                               DHT22 DATA ----------+        |   (to D4)
      |                                                             |
      +----------- DS18B20 GND        DS18B20 VCC ------------------+   (to 5V rail)
                                      DS18B20 DATA ----> D2 (with 4.7k pull-up to 5V)

   OLED VCC ----> 5V rail      OLED GND ----> GND rail
```

Signal summary:

```
 D2  <--- DS18B20 water probe (OneWire, 4.7k pull-up to 5V)
 D3  ---> active buzzer (+)
 D4  <--- DHT22 air temp + humidity (single-wire)
 A4  <--> OLED SDA (I2C data)
 A5  <--> OLED SCL (I2C clock)
 5V  ---> OLED, DS18B20, DHT22, 4.7k pull-up
 GND ---> OLED, DS18B20, DHT22, buzzer (-)
```

---

## 5. Buzzer logic (based on WATER temperature)

| Water temp | Buzzer | Screen | Serial |
|---|---|---|---|
| below 35 C | silent | (none) | `OFF` |
| 35 to 40 C | beep on/off every 400 ms | flashing `WARM` | `BEEP` |
| above 40 C | ON continuously | flashing `W HOT!` | `LOUD-CONT` |

Thresholds live at the top of the sketch:

```cpp
#define WARN_LIMIT 35.0     // beep from here
#define ALARM_LIMIT 40.0    // loud continuous above here
```

The buzzer is driven with `digitalWrite` **only** -- never `tone()`.
Mixing `tone()` and `digitalWrite` on the same pin jams the pin (the `tone()` timer
stays attached), which was an earlier bug. One method for all states fixes it.

A disconnected water probe reads as `OFF` and never false-triggers the alarm.

---

## 6. Display layout

```
  +--------------------------------------------+
  | W:24.5'                        FriedRust   |   <- water C, brand
  | A:22.0'                                    |   <- air C
  | H:48%           [ WARM ]                   |   <- humidity, warn box
  |         /\        __                       |
  |   ___/\/  \___   /LC78\___   /\/\          |   <- truck on scrolling ground
  |__/            \_/         \_/     \________|
  +--------------------------------------------+
```

- `W:` water C, `A:` air C, `H:` humidity % -- top-left.
- `FriedRust` brand -- top-right.
- `WARM` / `W HOT!` box flashes only when the buzzer is active.
- The truck hops when a boulder passes under it. Animation is non-blocking.

---

## 7. Serial output (9600 baud)

```
Water: 24.5C/76.1F   Air: 22.0C/71.6F  Hum:48%   Buzzer: OFF
Water: 37.2C/99.0F   Air: 22.1C/71.8F  Hum:47%   Buzzer: BEEP
Water: 92.3C/198.1F  Air: 22.1C/71.8F  Hum:47%   Buzzer: LOUD-CONT
```

Printed once every 2 s (paced by the DHT22, which can only read that often).

---

## 8. Read timing

| Sensor | Interval | Why |
|---|---|---|
| DS18B20 water | 0.5 s | fast, non-blocking request/read |
| DHT22 air | 2 s | DHT22 hardware limit (~0.5 Hz) |
| Display + buzzer | every loop (~40 ms) | smooth animation, responsive beep |

All reads are non-blocking (`millis()` timers), so the animation never stalls.

---

## 9. Libraries

Install via **Tools -> Manage Libraries**:

1. **Adafruit SH110X** (pulls in Adafruit GFX + Adafruit BusIO)
2. **Adafruit GFX Library**
3. **OneWire** (Paul Stoffregen)
4. **DallasTemperature** (Miles Burton)
5. **DHT sensor library** (Adafruit) -- say **yes** to Adafruit Unified Sensor

All of these are already vendored under [`../libraries/`](../../libraries).

---

## 9b. Build and upload from the command line (arduino-cli)

### Install the board core (one-time)

The UNO R4 WiFi uses the Renesas core. Install it once:

```sh
arduino-cli core update-index
arduino-cli core install arduino:renesas_uno
```

Check it is there: `arduino-cli core list` should show `arduino:renesas_uno`.

### Find your board's port

Plug the board in over USB, then:

```sh
arduino-cli board list
```

Example output:

```
Port                            Protocol Type              Board Name           FQBN                             Core
/dev/cu.usbmodemF412FA9C12345   serial   Serial Port (USB) Arduino UNO R4 WiFi  arduino:renesas_uno:unor4wifi    arduino:renesas_uno
```

The **Port** column is what you pass to `-p` below. By platform:

- **macOS**: looks like `/dev/cu.usbmodemXXXX`. List them with `ls /dev/cu.usbmodem*`.
- **Linux**: looks like `/dev/ttyACM0` (or `ttyUSB0`). You may need to be in the
  `dialout` group: `sudo usermod -aG dialout $USER`, then log out and back in.
- **Windows**: looks like `COM5`. Also visible in Device Manager under "Ports (COM & LPT)".

If the board does not appear: try another USB cable (some are charge-only), press the
board's reset button, and re-run `arduino-cli board list`.

### Compile and upload

Run these from the repo root. `--libraries ./libraries` uses the repo's vendored
libraries, so you do not have to install them by name.

```sh
# compile
arduino-cli compile \
  --fqbn arduino:renesas_uno:unor4wifi \
  --libraries ./libraries \
  sketches/water_monitor

# upload (replace the port with the one from 'arduino-cli board list')
arduino-cli upload \
  --fqbn arduino:renesas_uno:unor4wifi \
  --libraries ./libraries \
  -p /dev/cu.usbmodemXXXX \
  sketches/water_monitor
```

Watch the Serial output from the CLI (9600 baud, matches the sketch):

```sh
arduino-cli monitor -p /dev/cu.usbmodemXXXX -c baudrate=9600
```

---

## 10. Bring-up (step by step)

1. **Blink test** -- upload `sketches/blink_led/blink_led.ino`. The on-board LED path (D8) proves
   the toolchain and the board.
2. **Find the OLED** -- upload `sketches/i2c_address_finder/i2c_address_finder.ino`, open Serial at
   115200. It should report `0x3C`. If it reports something else, change `SCREEN_ADDRESS`.
3. **Wire the sensors** per the pin map above. Double-check the DS18B20 4.7 kO pull-up.
4. **Upload** `sketches/water_monitor/water_monitor.ino`. Open Serial at **9600**.
5. **Verify**: the OLED shows the truck + readouts; Serial prints the status line.
6. **Test the buzzer**: warm the water probe past 35 C (beeps), then past 40 C (steady).

---

## 11. Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| Blank OLED | wrong I2C address or SDA/SCL swapped | run the I2C scanner; A4=SDA, A5=SCL |
| `SH1106 allocation failed` on Serial | OLED not found on I2C | check 0x3C, wiring, 5V/GND |
| Water shows `W:--` | probe unplugged or missing pull-up | add 4.7 kO DATA->5V, check D2 |
| Air/Hum shows `--` | DHT22 wiring or too-fast reads | check D4, VCC, GND; keep 2 s interval |
| Buzzer never sounds | wrong pin or buzzer polarity | buzzer + to D3, - to GND |
| Buzzer stuck / weird | mixing `tone()` with `digitalWrite` | use `digitalWrite` only (active buzzer) |
| Values look wrong on first read | sensors still warming up | ignore the first second or two |

---

## 12. Tuning

- **Thresholds**: edit `WARN_LIMIT` / `ALARM_LIMIT`.
- **Beep speed**: change `400` in the beep block (ms per on/off toggle).
- **Animation speed**: change the final `delay(40)` and `scroll += 2` in `loop()`.
- **DS18B20 precision/speed**: `waterSensor.setResolution(10)` -- 9..12 bits.
