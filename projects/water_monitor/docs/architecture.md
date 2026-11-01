# water_monitor: how it runs

Hardware details (BOM, pin map, circuit, bring-up) are in [markdown/README.md](markdown/README.md). This page covers
what the firmware does at runtime.

## Hardware blocks

```mermaid
flowchart LR
  USB[USB 5V] --> MCU[UNO R4 WiFi<br/>RA4M1]
  DS[DS18B20 water probe] -- OneWire D2<br/>4.7k pull-up --> MCU
  DHT[DHT22 air temp + RH] -- D4 --> MCU
  MCU -- I2C A4/A5 @0x3C --> OLED[SH1106 1.3in 128x64]
  MCU -- D3 digitalWrite --> BZ[Active buzzer]
  MCU -- USB serial 9600 --> PC[Serial log]
```

## Boot (`setup()`)

1. `Serial.begin(9600)`, 300 ms settle.
2. Buzzer pin LOW. DS18B20 `begin()` at 10-bit resolution (~188 ms conversion). `dht.begin()`.
3. `display.begin(0x3C)`. **If it fails, the sketch prints `SH1106 allocation failed` and halts** (`for(;;)`).
4. Splash screen: "LC78 OFFROAD / Built by FriedRust" for 1.5 s.
5. Start the first DS18B20 conversion (`requestTemperatures()`).

## Main loop (~25 fps, `delay(40)`)

```mermaid
flowchart TD
  A[loop start] --> W{"&gt;500 ms since<br/>water read?"}
  W -- yes --> W1["getTempCByIndex(0)<br/>(result of previous conversion)<br/>then requestTemperatures()"]
  W -- no --> R
  W1 --> R{"&gt;2000 ms since<br/>air read?"}
  R -- yes --> R1[DHT22 temp + RH<br/>print serial log line]
  R -- no --> B
  R1 --> B[Buzzer state machine]
  B --> D[Draw frame: terrain, boulder, car,<br/>dust, readout, brand, WARN box]
  D --> P["display.display()<br/>scroll += 2, boulder -= 3"]
  P --> Z["delay(40)"] --> A
```

The water reading lags one cycle (≤ 500 ms) because each read returns the conversion requested the previous time.
This keeps the loop non-blocking.

## Buzzer state machine (water temperature)

```mermaid
stateDiagram-v2
  [*] --> Silent
  Silent --> Beep: 35.0 ≤ water ≤ 40.0
  Silent --> Alarm: water > 40.0
  Beep --> Alarm: water > 40.0
  Beep --> Silent: water < 35.0 or probe lost
  Alarm --> Beep: 35.0 ≤ water ≤ 40.0
  Alarm --> Silent: water < 35.0 or probe lost
  Silent: D3 LOW
  Beep: D3 toggles when >400 ms since last toggle (~440 ms with 40 ms loop)
  Alarm: D3 HIGH steady
```

Probe disconnected (`DEVICE_DISCONNECTED_C`) means silent. **Missing probe = no alarm**, which is a design choice
to revisit if this guards anything critical.

## Screen

| Area | Content |
|---|---|
| top-left 62x30 | `W:24.5°` / `A:22.0°` / `H:48%`, or `--` when a sensor fails |
| top-right | `FriedRust` |
| centre (flashing every 4 frames) | `WARM` (beep) or `W HOT!` (alarm) box |
| bottom | scrolling sine terrain, boulder, LC78 that lifts over the boulder |

## Serial log (every ~2 s)

```
Water: 24.5C/76.1F   Air: 22.0C/71.6F  Hum:48%   Buzzer: OFF
Water: --   Air: --   Buzzer: OFF          (sensor failures)
```

`Buzzer:` is one of `OFF`, `BEEP`, `LOUD-CONT`. The native tests and the Wokwi scenario both assert on this format.
Changing it means updating both.

## Timing budget

| Task | Period | Blocking time |
|---|---|---|
| DS18B20 read + new conversion | 500 ms | ~few ms (OneWire) |
| DHT22 read + log | 2000 ms | ~5 ms |
| Frame draw + I2C push (1 KB) | every loop | ~25 ms at 400 kHz I2C |
| `delay(40)` | every loop | 40 ms |

## Sim vs real

| Real | Wokwi | Native |
|---|---|---|
| UNO R4 WiFi (RA4M1) | AVR Uno (ATmega328P) | host clang++ |
| SH1106 | SSD1306 via shim | text capture |
| DS18B20 / DHT22 | Wokwi parts | `sim::ds18b20C`, `sim::dhtTempC` |
| active buzzer | Wokwi buzzer on D3 | `sim::pinLevel[3]` |
