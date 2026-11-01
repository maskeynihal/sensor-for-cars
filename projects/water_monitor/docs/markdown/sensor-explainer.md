# How the temperature sensors work — from analog physics to °C/°F

This explains the full chain for both sensors in this project:

```
physical heat → analog voltage inside the chip → ADC → digital number
→ 1-wire protocol on the pin → Arduino library decodes → your °C/°F math
```

A key point up front: **both your sensors are digital on the wire.** The
DS18B20 and the DHT22 each do the analog measurement and the analog-to-digital
conversion *inside their own silicon*, then send you a ready-made number over a
single data pin. You never see a raw analog voltage on an Arduino analog pin for
these. So "analog viewpoint" here means **what happens inside the chip** before
it hands you a digital value — and that is where the real sensing physics lives.

For contrast, section 5 covers a classic **analog** sensor (LM35 / thermistor)
where the Arduino's own ADC does the work, because that is the case most people
mean by "analog temperature sensing".

---

## 1. The physics: how heat becomes a voltage

Every temperature sensor converts heat into an electrical quantity. Three
common mechanisms:

| Mechanism | Part that uses it | What changes with temperature |
|---|---|---|
| Silicon bandgap | DS18B20, DHT22, LM35, TMP36 | the forward voltage of a diode / BJT junction |
| Thermistor (NTC) | cheap probes, 3D printers | the **resistance** of a ceramic bead |
| Thermocouple | kilns, ovens | a tiny voltage from two dissimilar metals (Seebeck effect) |

### The silicon bandgap effect (what the DS18B20 uses)

Inside the chip there is a transistor junction. The voltage across a
forward-biased silicon junction drops in a very predictable, nearly linear way
as temperature rises — about **−2 mV per °C**. If you run two junctions at
different current densities and subtract, the *difference* in their voltages is
**proportional to absolute temperature** (this is the "PTAT" voltage —
Proportional To Absolute Temperature). That PTAT voltage is the analog signal
the chip actually measures.

```
V_junction
   ^
   |\
   | \____           higher temp  → lower junction voltage
   |      \____       (about -2 mV per degree C)
   |           \____
   +------------------> temperature
```

So the "sensor" is really: a known current pushed through a junction, producing
a voltage that tracks temperature. Everything after that is turning that
analog voltage into a clean number.

---

## 2. Inside the DS18B20: analog → digital, all on-chip

The DS18B20 is a whole measurement system in a TO-92 package (the probe's metal
tip). Signal chain inside it:

```
 [junction/PTAT]      analog voltage proportional to temperature
        |
        v
 [analog front-end]   amplifies + references the PTAT voltage
        |
        v
 [sigma-delta ADC]    converts the analog voltage to a digital count
        |
        v
 [temperature register]  a signed 16-bit number, units of 1/16 °C
        |
        v
 [1-Wire interface]   sends those bytes out on the single data pin
```

### The ADC and resolution

The DS18B20 uses a **sigma-delta ADC**. It compares the PTAT voltage against an
internal reference and counts, building up a digital value. The number of bits
it resolves is configurable — and this is exactly what your code sets:

```cpp
waterSensor.setResolution(10);   // line 159 of water_monitor.ino
```

| Resolution | Step size | Conversion time | Your setting |
|---|---|---|---|
| 9-bit | 0.5 °C | ~94 ms | |
| **10-bit** | **0.25 °C** | **~188 ms** | ← this project |
| 11-bit | 0.125 °C | ~375 ms | |
| 12-bit | 0.0625 °C | ~750 ms | |

More bits = finer steps but a **longer conversion time**. 10-bit (0.25 °C) is a
good balance for water temperature — you do not need 0.06 °C precision to decide
"is the water over 40 °C", and the shorter conversion keeps the loop snappy.

### What the register actually holds

The chip stores temperature as a **signed 16-bit integer in units of 1/16 °C**
(that is its native format, regardless of the resolution you picked — lower
resolutions just zero the low bits). So:

```
raw value 0x0191 = 401 decimal
401 / 16 = 25.0625 °C
```

The library does this division for you. That is why `getTempCByIndex` returns a
`float` like `25.06`, not a raw integer.

---

## 3. The 1-Wire protocol: how the number crosses the pin

The DS18B20 talks over Dallas **1-Wire** — one data line (your **D2**) carries
both directions, timed to the microsecond. This is why it needs the **4.7 kΩ
pull-up** to 5 V: the line idles HIGH, and both the Arduino and the chip talk by
briefly pulling it LOW. The pull-up restores it to HIGH; without the resistor the
line floats and the protocol fails (this is why a missing pull-up shows `W:--`).

A reading is a two-step dance, and your code follows it exactly:

```cpp
waterSensor.requestTemperatures();     // line 177 (setup) and 196 (loop)
...
float t = waterSensor.getTempCByIndex(0);   // line 190
```

1. **`requestTemperatures()`** sends the 1-Wire "convert T" command. The chip
   starts its sigma-delta conversion. This takes ~188 ms at 10-bit.
2. **`getTempCByIndex(0)`** reads the result bytes from the first sensor on the
   bus (index 0), checks the CRC, and returns °C as a float.

### Why the code requests *then* reads on the NEXT pass

Look at the order in `loop()`:

```cpp
if (millis() - lastWater > 500) {
  float t = waterSensor.getTempCByIndex(0);  // read the result of the LAST request
  ...
  waterSensor.requestTemperatures();         // kick off the NEXT conversion
  lastWater = millis();
}
```

It reads **first**, then requests. This is deliberate and non-blocking: the
conversion kicked off 500 ms ago is already finished, so the read is instant;
then it starts a fresh conversion that will be ready by the next pass. If you
instead did `requestTemperatures()` and immediately `getTempCByIndex()`, you
would either block for ~188 ms (freezing the animation) or read a stale value.
The 500 ms interval is comfortably longer than the 188 ms conversion, so the
value is always ready.

### Detecting a disconnected probe

```cpp
if (t != DEVICE_DISCONNECTED_C) { ... waterOK = true; }
else waterOK = false;
```

`DEVICE_DISCONNECTED_C` is a sentinel value (−127 °C) the library returns when
the CRC fails or no device answers. The code treats that as "not OK" so a
missing probe shows `W:--` and never false-triggers the buzzer.

---

## 4. Inside the DHT22: the same idea, different packaging

The DHT22 (AM2302) also measures analog quantities internally and sends digital
data, but it bundles **two** sensors:

- **Humidity**: a capacitor whose dielectric absorbs water vapour. More moisture
  → higher capacitance. The chip measures that capacitance (an analog value) and
  converts it to a %RH number.
- **Temperature**: a bandgap/thermistor element, same principle as section 1.

Its on-board chip digitises both and sends them over a **single-wire** protocol
(not Dallas 1-Wire — a simpler custom timing) on your **D4**.

### The wire format

One reading is **40 bits**: 16 bits humidity, 16 bits temperature, 8 bits
checksum. The temperature is a signed integer in **units of 0.1 °C**:

```
raw temp bytes = 0x00E6 = 230 decimal
230 / 10 = 23.0 °C
```

The DHT library handles the bit-banged timing, assembles the 40 bits, verifies
the checksum, and does the ÷10. Your code just asks:

```cpp
float t = dht.readTemperature();   // line 202 — returns °C
float h = dht.readHumidity();      // line 203 — returns %RH
```

### Why the DHT22 is read only every 2 seconds

```cpp
if (millis() - lastAir > 2000) { ... }
```

The DHT22's internal sampling is slow — roughly **0.5 Hz** (one new reading
every ~2 s). Polling faster just returns the same cached value or a read error
(`NaN`). The code respects that hardware limit, which is why the Serial line
prints every 2 s. Failed reads come back as `NaN`, caught here:

```cpp
if (!isnan(t) && !isnan(h)) { ... airOK = true; }
else airOK = false;
```

---

## 5. The "true analog" case (for comparison): LM35 / thermistor

This project does **not** use one, but this is what people usually mean by
"analog temperature sensing", and it shows where the Arduino's own ADC comes in.

### LM35 — voltage out

An LM35 outputs a clean analog voltage: **10 mV per °C** (25 °C → 0.25 V). You
wire its output to an Arduino **analog** pin (A0…) and the Arduino's built-in
**10-bit ADC** does the conversion your DS18B20 did internally:

```cpp
int raw = analogRead(A0);              // 0..1023 (10-bit ADC)
float volts = raw * (5.0 / 1023.0);    // ADC count → volts (5V reference)
float celsius = volts * 100.0;         // 10 mV/°C  → ÷0.01 = ×100
```

- `analogRead` returns **0..1023** because the ADC is 10-bit (2¹⁰ = 1024 steps).
- Each step = reference ÷ 1024. At a 5 V reference that is **~4.9 mV per count**.
- With the LM35's 10 mV/°C, one ADC count ≈ **0.49 °C** of resolution.

This is the whole reason sensors like the DS18B20 moved the ADC *inside* the
chip: an on-chip sigma-delta ADC with an internal reference is far more precise
and noise-immune than reading a long analog wire into the Arduino's ADC, where
wire resistance, noise pickup and the 5 V rail's own wobble all corrupt the
reading.

### Thermistor — resistance out

A thermistor changes resistance, so you put it in a voltage divider with a fixed
resistor, read the midpoint voltage with `analogRead`, back out the resistance,
then apply the **Steinhart–Hart equation** to get temperature. More math, because
resistance-vs-temperature is non-linear. The DS18B20 does all of this for you.

---

## 6. The °C → °F conversion (the only analog-free step)

Once either sensor has handed you a `float` in °C, Fahrenheit is pure
arithmetic. Your code, lines 193 and 206:

```cpp
waterF = t * 9.0 / 5.0 + 32.0;   // water probe
airF   = t * 9.0 / 5.0 + 32.0;   // air sensor
```

This is the textbook formula **°F = °C × 9/5 + 32**. Worked example at 25 °C:

```
25 × 9 / 5 + 32
= 225 / 5 + 32
= 45 + 32
= 77.0 °F
```

Two details that matter in C++:

- **Write `9.0 / 5.0`, not `9 / 5`.** `9 / 5` is *integer* division and equals
  `1` (the remainder is thrown away), which would make the formula badly wrong.
  The `.0` forces floating-point division so you get `1.8`. This is a classic
  C/C++ bug and the code gets it right.
- **`t` is already a `float`**, so the whole expression is evaluated in floating
  point and `waterF` keeps the fractional part (e.g. `76.1`).

---

## 7. The full journey of one reading (DS18B20)

Putting it all together, here is what happens for a single water-temperature
number from the moment heat hits the probe to the `76.1F` on your screen:

```
1. Warm water heats the probe tip.
2. The junction voltage inside the DS18B20 drops ~2 mV per °C (analog).
3. The sigma-delta ADC converts that voltage to a 16-bit count (1/16 °C units),
   to 10-bit precision (0.25 °C steps) because setResolution(10).
4. requestTemperatures() told it to do this ~500 ms ago; it is done.
5. getTempCByIndex(0) clocks the bytes out over 1-Wire on D2 (4.7k pull-up
   holds the line), checks the CRC, divides by 16, returns 25.06 as a float.
6. The code checks it != DEVICE_DISCONNECTED_C → waterOK = true.
7. waterF = 25.06 * 9.0/5.0 + 32.0 = 77.1 °F.
8. drawReadout() prints "W:25.0" ; Serial prints "Water: 25.1C/77.1F".
9. The buzzer logic compares 25.06 against WARN_LIMIT/ALARM_LIMIT.
```

Every analog step (2, 3) is inside the chip; everything from step 5 on is the
digital number your code works with.

---

## 8. Quick reference

| | DS18B20 (water) | DHT22 (air) |
|---|---|---|
| Sensing element | silicon bandgap junction | bandgap temp + capacitive humidity |
| Where the ADC lives | inside the chip | inside the chip |
| On-wire format | Dallas 1-Wire, signed, 1/16 °C units | custom single-wire, 40 bits, 0.1 °C units |
| Needs pull-up? | yes, 4.7 kΩ on DATA | breakout has its own |
| Resolution here | 10-bit → 0.25 °C | 0.1 °C |
| Max read rate | ~5 Hz at 10-bit | ~0.5 Hz (every 2 s) |
| Arduino pin | D2 | D4 |
| Library call | `getTempCByIndex(0)` | `readTemperature()` / `readHumidity()` |
| Error sentinel | `DEVICE_DISCONNECTED_C` (−127) | `NaN` |
| °F formula | `t * 9.0/5.0 + 32.0` | `t * 9.0/5.0 + 32.0` |
