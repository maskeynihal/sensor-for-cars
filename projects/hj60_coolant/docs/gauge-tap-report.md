# HJ60 coolant temperature: the measurements, the errors, and the plan

- Date: 2026-11-01
- Vehicle: Toyota Land Cruiser HJ60, 2H 4.0 L diesel.
- Electrical system: 12 V, one battery. The owner measured 12.9 V with the engine off and 14.0 V with the engine
  running (2026-11-01).
- An earlier draft said 24 V. That was wrong for this truck. Some HJ60 trucks (Canada, Europe, ex-JDM) have a 24 V
  system. For that reason, the FSM shows a "Resister (24V)".
- Goal: read the engine coolant temperature with an ESP32 DevKit V1. Use the sender that is already in the engine.
  Later, add our own probe as a second check.
- Functions: an alarm and a screen first. Then logging and limits that the driver can change.
- Primary source: Toyota FSM, Body Electrical, pages 13-73 and 13-74 (photos in [img/](img/)).
- If this circuit is new to you, start with the interactive explainer:
  [html/circuit-explainer.html](html/circuit-explainer.html).

## Words in this report

| Word | Meaning |
|---|---|
| Sender | The temperature sensor in the cylinder head. It is a resistor. Its resistance decreases when the coolant gets hotter (NTC thermistor). |
| Gauge | The temperature needle on the dash. A small heater coil bends a bimetal strip, and the strip moves the needle. |
| Regulator | A bimetal switch in the fuel gauge. It opens and closes again and again to keep the average gauge voltage constant. |
| E, F | The two terminals of the gauge heater coil. E connects to the regulator. F connects to the sender wire. |
| Feed | The voltage at terminal E. |
| Sender wire | The yellow-green (YG) wire from terminal F to the sender. |
| Rs | Sender resistance. |
| Rg | Gauge heater resistance, between E and F. The FSM value is 25 Ω. |
| k | The ratio of sender-wire voltage to feed voltage. |
| Tap | A high-resistance connection that lets the ESP32 measure a voltage without an effect on the circuit. |
| ADC | The part of the ESP32 that measures voltage. |

## 1. Summary

1. Your meter readings were correct. The interpretation was wrong. The gauge circuit gets its voltage in pulses from
   a bimetal regulator. A multimeter cannot show a steady number on a pulsed voltage. The FSM test for this circuit
   expects a flashing bulb.
2. You measured 150 Ω at the disconnected sender with a warm engine. That value agrees with about **60 °C**. That
   temperature agrees with an engine that ran a short time before.
3. The ohms reading changed continuously because the sender was **still connected** to the gauge. The meter then
   measured the sender, the gauge heater, the regulator contacts and the other parts on that circuit together. An
   ohms reading is correct only on a part that is disconnected and has no power.
4. The regulator caused the **~10 V at key-on, then the changing voltage**. When the regulator is cold, its contacts
   are closed. Then the sender wire stays near the full feed voltage. When the bimetal gets hot, the contacts open and
   close again and again. The meter samples this square wave a few times each second and shows random numbers.
5. The solution is to measure the **ratio**, not the voltage. Measure the feed (terminal E) and the sender wire
   (terminal F) at the same time. The sender and the gauge heater make a resistive divider, so
   `Rs = Rg × Vs / (Vfeed − Vs)`.
6. This equation stays correct when the regulator pulses, when the battery voltage changes between 12 V and 14.5 V,
   and with a solid-state regulator. The ESP32 takes almost no current from the circuit, so the dash gauge continues
   to operate correctly.
7. The water_monitor prototype (UNO R4 + DS18B20) is not safe to install in this truck in its current form. See
   section 5.
8. There are two other connection options. Option B disconnects the gauge, and the ESP32 measures the sender
   directly. Option C also lets the ESP32 operate the gauge. See section 8.

## 2. The FSM data (HJ60 = "BJ, HJ Series")

From [13-73](img/fsm-13-73-water-temp-gauge-circuit.png) and
[13-74](img/fsm-13-74-gauge-and-sender-inspection.png):

```
 12 V batt ── ignition relay ── ENGINE fuse 15 A ── (Resister, 24 V trucks only) ──► A ┐
                                                                        │ voltage regulator
                                                       (inside the fuel receiver gauge, bimetal contacts)
                                                                        │
                                                       regulated feed ──┼──► fuel gauge heater ── fuel sender ── ⏚
                                                                        │
                                                                        └──► E ┐ water temp gauge
                                                                               │ bimetal heater, E–F = 25 Ω (STD)
                                                                               F ┘
                                                                               │ wire (YG)
                                                                               ▼
                                                                    water temperature sender
                                                                    (NTC, body grounded to engine) ── ⏚
```

| FSM item | Value | Page |
|---|---|---|
| Gauge type | Bimetal (thermal) receiver | 13-74 "Inspection (Bimetal Type)" |
| Gauge heater resistance E–F | **STD 25 Ω** | 13-74 |
| Regulator | In the fuel receiver gauge | 13-74 |
| Feed on 24 V trucks | Through a series "Resister (24V)" after the 15 A ENGINE fuse. **This truck is 12 V.** Thus we do not expect a resistor. Check this in step M0. | 13-73 BJ/HJ diagram |
| Voltage at terminal A | "2–7 V" with the ignition on. This text is not clear. See the note below. | 13-74 |
| Receiver test | Connect the sender wire to ground through a 3.4 W bulb. The bulb **flashes** and the needle **vibrates**. | 13-73 |
| Sender | 50 °C → 226 Ω, 115 °C → 26 Ω | 13-74 table |

Note: the diagram shows terminal A as the regulator *input*. Thus the "2–7 V at terminal A" text does not agree with
the diagram. Possibly it means the average value that a meter shows. The ratio method does not use this value. This
report does not try to solve this problem.

Other HJ60 data. These come from vendors and forums, so they are less reliable than the FSM:

- Thermostat 90916-03026 starts to open at 82 °C. It is fully open at 95 °C
  ([allfourx4](https://www.allfourx4.com.au/Thermostat-suitable-for-Landcruiser-H-2H-3B-HJ-BJ-40-60-75-Genuine-90916-03026)).
- Radiator cap: 0.9 kg/cm² (about 88 kPa, 13 psi)
  ([allfourx4/Tridon](https://www.allfourx4.com.au/Radiator-Cap-suitable-for-Prado-Landcruiser-Hilux-Surf-4Runner-Patrol-Navara-Tridon-CB1390/)).
  With this cap, a 50/50 glycol mixture boils at about 124–126 °C. Water without glycol boils at about 118 °C. We
  calculated these two values.
- Normal coolant temperature is about 85–88 °C
  ([ih8mud](https://forum.ih8mud.com/threads/oem-temperature-gauge-fault.1385755/)).
- 2H cylinder heads crack after overheating ([ih8mud](https://forum.ih8mud.com/threads/a-2h-story.757492/)).
- The sender has an M16×1.5 thread and one terminal. It is in the cylinder head. Cross-references:
  83420-20011, 83420-20020, 83420-16010
  ([euro4x4parts](https://www.euro4x4parts.com/en/parts/ref=KSD1008/sender-coolant-temperature/oe=83420-20011_mc850070_82180-4150_83420-20020_21203-aa000_83420-16010)).
- **Do not mix up** the sender and the glow-plug control sensor 89422-30020. That sensor has 2 pins. It is near the
  thermostat housing. It reads about 2 kΩ at 21 °C
  ([allfourx4](https://www.allfourx4.com.au/Oil-Temperature-Sensor-suitable-for-Landcruiser-2H-12HT-Diesel-89422-30020A),
  [ih8mud](https://forum.ih8mud.com/threads/2h-wiring-question.83647)).
- The 2H has no ECU and no OBD port. It has a mechanical injection pump. Thus the gauge sender is the only coolant
  temperature signal from the factory.

## 3. What the measurements mean

### 3.1 Sender to ground, disconnected: 150 Ω

The FSM gives two points. We fitted a Beta (NTC) curve through them, with B ≈ 4173 K. **This curve is an
approximation.** The two points are 65 °C apart, and we extrapolated outside them. Calibrate the curve in step M4.

| °C | Rs (Ω) | k = Rs/(Rs+25) |
|---|---|---|
| 20 | ~847 | 0.971 |
| 50 | 226 (FSM) | 0.900 |
| **60** | **~153** | 0.860 |
| 82 (thermostat starts to open) | ~71 | 0.738 |
| 90 | ~55 | 0.686 |
| 100 | ~40 | 0.616 |
| 105 | ~35 | 0.580 |
| 110 | ~30 | 0.545 |
| 115 | 26 (FSM) | 0.510 |
| 120 | ~23 | 0.476 |

150 Ω agrees with **about 60 °C**. That agrees with "warm, ran a short time before". The sender is probably
serviceable. With a cold engine at 20 °C, the sender must show approximately 850 Ω. Measure this value in step M4.

**The error can be large until you do step M4.** A forum post about the FJ60 (petrol) sender gives 27 Ω at 100 °C and
23 Ω at 105 °C ([ih8mud](https://forum.ih8mud.com/threads/how-i-fixed-my-temp-gauge.1014115/)). Our Beta curve
converts these resistances to about 114 °C and 119 °C. Thus the two sources have a difference of about 14 °C at the
WARN and ALARM limits. The engine and the sender are different, but this shows that a two-point curve is not
accurate. **Before the M4 calibration, readings near 100 °C can have an error of more than ±10 °C. Do not trust WARN
or ALARM yet.**

### 3.2 Ohms with the gauge connected: the value changes

On the ohms range, a multimeter sends a small current and measures the voltage. When the sender is connected to the
gauge, a second path is in parallel with the sender. This path goes through the 25 Ω gauge heater, the regulator
contacts and the fuel gauge circuit. The path changes when the bimetal contacts move and cool. With the key on, the
circuit voltage also causes errors in the meter. The reading has no meaning. It is not a fault.

**Rule: measure resistance only with the sender wire disconnected and the key off.**

### 3.3 Volts at the sender wire: 0 V key off, ~10 V for some seconds, then random values

- Key off: no feed, 0 V. This is correct.
- Key on: the regulator bimetal is cold, so its contacts are closed. The feed is at its "on" level. The sender wire
  voltage is `Vfeed × Rs/(Rs+25)`.
- With Rs ≈ 150 Ω, k = 0.86. Thus 10 V at the sender wire agrees with **11.7 V of feed** during the on time.
- 11.7 V is the 12.9 V battery voltage minus the voltage drops in the ignition relay, fuse, wiring and regulator
  contacts. With the engine running (14 V), the on level will probably increase to about 12.5–13 V. The ratio method
  is not affected by this change.
- After some seconds, the regulator heater makes its bimetal hot. The contacts open, and the sender wire goes to
  approximately 0 V. The bimetal cools and the contacts close again. Thus the feed becomes a series of pulses.
- The gauge is also a slow heater, so it uses the average of the pulses to move the needle. This is the FSM
  bulb-flashing test.
- A typical multimeter takes 2 to 5 readings each second, at times that are not related to the pulses. Each reading
  occurs at a different point of the on/off cycle. Thus the meter screen shows a different value each time.
- **A DC meter cannot measure a pulsed voltage correctly.** Use an oscilloscope, or the ESP32 capture sketch in step
  M2.

### 3.4 Why the ratio method works

The gauge heater (E–F) and the sender make a voltage divider. The regulator supplies this divider:

```
Vfeed (E) ──[ Rg = 25 Ω ]── F = Vs ──[ Rs ]── ⏚
k = Vs / Vfeed = Rs / (Rs + Rg)     ⇒     Rs = Rg × k / (1 − k)
```

- The divider has only resistors, and it changes at the same time as the feed. Thus k has the same value at each
  moment when the feed is on. The feed level (7 V, 11.7 V or 14 V) and the pulse shape have no effect on k.
- When the feed is off, the two voltages are approximately 0 V. The ESP32 ignores those samples.
- An error in Rg causes the same error in Rs. An error of 1 Ω in Rg causes a 4% error in Rs. **Measure E–F on the
  cluster of this truck (step M1). Do not use "25 Ω STD" without a check.**
- Ground offset: the sender connects to ground at the engine block. The ESP32 connects to ground in the cab. A
  difference of some tens of mV between these grounds adds an error of about 0.5 °C. Connect the ESP32 ground at the
  cluster, and measure the offset (step M3).

## 4. How to connect the ESP32 (DevKit V1)

### 4.1 Signal taps: two high-resistance dividers, one for each channel

Connect one tap to each end of the gauge heater coil: **E** (feed) and **F** (sender wire). Use the same circuit for
the two taps.

**Find E and F first (step M0).** The FSM shows the cluster connector pins as D/B/A/13/1/G (Fig 13-148), not as E/F.
The regulator is in the fuel gauge. The feed is possibly not available at the connector. Thus you may have to open
the cluster and connect the tap to the terminals of the gauge.

- **Do not use connector pin A as the feed tap.** Pin A is the regulator *input*. Pin A stays at full voltage while
  the regulator is off, but the sender wire goes to approximately 0 V. The firmware then accepts these samples with
  k ≈ 0. It then shows FAULT-SHORT or an incorrect hot temperature.
- Check 1: key off, cluster disconnected. The resistance between the two tap points must be **about 25 Ω** (the gauge
  heater).
- Check 2: key on. The feed tap voltage must never be lower than the sender tap voltage.

```
 tap ──[ 68 kΩ 0.1% ]───┬───────────┬──────► ESP32 ADC1 pin (GPIO34 = feed, GPIO35 = sender)
                        │           │
                     [10 kΩ 0.1%] [100 nF]        BAT54S: clamps to 3V3 and GND at the ADC pin
                        │           │
                       ⏚ (cluster ground = ESP32 GND)
```

- Divider ratio: 7.8:1, for the 12 V system. The worst case is 16 V, from a defective alternator regulator. The
  divider changes 16 V to 2.05 V. At 11 dB attenuation, the ESP32 ADC is accurate from about 0.15 V to 2.45 V, so
  2.05 V is in this range.
- The measured on level of ~11.7 V becomes ~1.5 V at the ADC. The clamp diode limits voltage spikes above this level.
- Sensitivity near 100 °C: k changes by about 0.007 for each °C. With an 11.7 V feed, this is about 10.5 mV for each
  °C at the ADC. An average of 64 to 128 samples gives a resolution of about 0.2 to 0.3 °C.
- Load on the gauge circuit: 78 kΩ in parallel with a 25 to 200 Ω circuit. The effect is less than 0.3%. The needle
  does not move.
- The 100 nF capacitor gives a time constant of approximately 0.9 ms. It removes contact bounce. It is much faster
  than the regulator cycle (seconds).
- Use 0.1% resistors in the two channels. Matched dividers cancel in k. Alternatively, calibrate with a resistor of known value
  in place of the sender (step M5).
- **Never** connect a low-resistance part to a tap. The FSM uses a 3.4 W bulb, not a direct short, because a short
  can burn the gauge heater. The same rule applies to multimeter probes: do not measure ohms on a circuit with power.
- Use only ADC1 pins (GPIO32 to GPIO39). ADC2 does not operate when WiFi is on.
- **The ESP32 ADC offset is the largest source of error.** Even with the factory calibration
  (`analogReadMilliVolts`), the offset is approximately ±20–30 mV. At a 1.5 V feed, this gives up to about 2% error in
  k, or 2–3 °C. Calibrate with resistors of known value (step M5). If necessary, add an offset constant to the firmware.

### 4.2 Power: 12 V truck → 5 V ESP32

The ESP32 DevKit V1 needs 5 V on its VIN/5V pin. **Do not** connect 12 V to it. Install the power circuit in this
sequence (details in [ADR 0009](../../../docs/adr/0009-hj60-12v-power-front-end.md), Proposed):

1. Install a 1 A **fuse** at the connection point. Use a circuit that the ignition switch controls.
2. Install **reverse-polarity protection**: a Schottky diode or an ideal-diode controller.
3. Install a **TVS diode** across the input. Use a standoff voltage of about 26–28 V, so that a 24–26 V jump start
   does not make it conduct. Its clamp voltage (about 45 V) must be less than the maximum input of the converter.
4. Install a **buck converter with a rating of at least 60 V input** (40 V is the absolute minimum) and a 5 V output.
   Connect its output to the 5V pin of the DevKit. The converter must keep 5 V with an input as low as about 6 V.
5. Connect the **ground at the cluster**.

During cranking, the voltage can decrease enough to reset the ESP32. This is acceptable. The ESP32 starts again in
about 1 second, in the `WAIT` state.

12 V data (from vendor application notes that quote the standards):

- ISO 16750-2 load dump, Test A (unsuppressed): 79–101 V for 40–400 ms
  ([TI SNOAAA1](https://ti.com/document-viewer/lit/html/SNOAAA1)).
- Test B (clamped): 35 V
  ([ADI](https://www.analog.com/en/resources/technical-articles/low-quiescent-current-surge-stopper-robust-automotive-supply-protection.html)).
- During cranking, the voltage decreases to approximately 6 V
  ([MPS](https://media.monolithicpower.com/mps_cms_document/c/o/cold_crank_to_load_dump-primer_ver1.3.pdf)).
- Jump start: 24–26 V for 60 s. Reverse battery: −14 V for 60 s
  ([ATEC summary](https://www.atecorp.com/compliance-standards/iso/ISO-16750-2)).
- ISO 7637-2 12 V pulses: 1 = −75/−100 V, 3b = +75/+150 V
  ([Littelfuse](https://www.mouser.com/pdfdocs/Littelfuse_TVS_Diode_Automotive_Circuit_Protection_Using_Automotive_TVS_Diodes_Application_Notepdf.pdf)).

### 4.3 Outputs

- 1.3" SH1106 OLED on I2C (GPIO21 SDA, GPIO22 SCL). The OLED shows only static text, with no animation (section 5).
- Active buzzer on GPIO25 **through an NPN transistor** (for example 2N2222 or S8050, with a 1 kΩ base resistor and a
  flyback diode). A 5 V active buzzer needs more current than an ESP32 pin can safely supply.

## 5. What was wrong in water_monitor (for this use)

water_monitor was a bench project for "a tank of water". As an HJ60 coolant monitor, it has these problems:

| # | Problem | Effect in the HJ60 | Correction |
|---|---|---|---|
| 1 | Limits of 35 / 40 °C | Normal coolant temperature is 82–95 °C. The alarm would operate on each drive. | Warn at ≥100 °C, alarm at ≥105 °C, with 2 °C hysteresis. Let the driver change them (phase 3). |
| 2 | Missing probe = no sound | A failed sensor looks like "all correct". The alarm fails without a warning. | Make faults loud: an open sender, a shorted sender or no feed is a **fault**, with its own sound and screen. |
| 3 | DS18B20 probe with PVC cable | PVC cable has a rating of about 80–85 °C. The chip limit is 125 °C, near the 124 °C boiling point with pressure. Accuracy is ±0.5 °C only from −10 to +85 °C. | Use the factory sender as the primary sensor. For a second sensor, use a DS18B20 with PTFE cable in a thermowell in a hose adapter. Do not put a bare probe in the pressurised coolant. |
| 4 | DS18B20 power-on value of 85.0 °C | 85.0 °C is a normal coolant temperature. Thus a reset error looks like a correct value. | If you use a DS18B20, reject 85.0 exactly after a conversion error. |
| 5 | UNO R4 on USB, or on VIN with no transient protection | VIN (6–24 V) has only Schottky diodes and no TVS. A 12 V truck has load dumps of 35–101 V and cranking voltages down to about 6 V. | Automotive power circuit with a 5 V output (4.2). |
| 6 | No transient, reverse or fuse protection | Load dump, jump start, cranking. | See 4.2. |
| 7 | Breadboard and Dupont jumpers | Vibration from 10 to 2000 Hz causes wear on contacts with low force. | Soldered perfboard or PCB, locking connectors, strain relief. |
| 8 | Animated truck picture | It pulls the eyes of the driver. NHTSA guidance: 2 s for each glance and 12 s for each task. OLED life is about 10,000 h at full brightness. | Large static number, low brightness, screen off when idle, flash only for an alarm. |
| 9 | DHT22 on the dash | A dash in the sun can go above the 80 °C rating. | Remove it, or install it in the shade. Coolant monitoring does not need it. |
| 10 | Diagnosis with only a voltmeter and an ohmmeter | The circuit voltage comes in pulses (3.3). | ESP32 capture sketch (M2) and the ratio method. |
| 11 | Ohms measured with the circuit connected | The reading has no meaning (3.2). | Disconnect the wire and turn the key off. |
| 12 | The sketch stops if `display.begin()` fails (`for(;;)`) | A loose OLED connector also stops the alarm. | Record the error and continue (done in hj60_coolant). |

This problem remains with the new design: **coolant loss**. If the coolant level goes below the sender, the sender is
in steam or air and reads a *lower* temperature. Then the gauge and our ESP32 can show a decreasing temperature while
the engine overheats. Possible solutions (next steps, not built):

- A check on the rate of temperature change.
- A second sensor lower in the circuit or on the upper hose.
- A coolant level probe.

## 6. Plan

### Phase M: measurements on the truck (do these before you trust the numbers)

**M0. Find the tap points.**

1. Turn the key off. Remove the cluster.
2. Find two points with about 25 Ω between them. These are terminals E and F of the temperature gauge. They are
   possibly inside the cluster.
3. Do **not** use connector pin A.
4. Turn the key on. Check that the feed point voltage is always equal to or more than the sender point voltage.
5. Record the position of E and F. Take a photo.

**M1. Measure the gauge heater resistance (Rg).**

1. Turn the key off. Disconnect the cluster.
2. Measure the resistance between E and F of the temperature gauge.
3. Write the value into `GAUGE_R_OHMS` in the firmware.

**M2. Record the feed waveform.**

1. Build the two dividers (4.1).
2. Load `projects/_utilities/esp32_gauge_capture` into the ESP32.
3. Turn the key on with a cold engine. Record 30 s.
4. Start the engine and let it become warm. Record 30 s.
5. From the data, find the on level (V), the period, the duty cycle and the contact bounce.
6. Set `FEED_ON_V` below the on level.
7. If the on pulses are short (less than ~50 ms in each second), decrease `SAMPLE_MS` or `MIN_PAIRS`. If you do not
   do this, the firmware will show a false FAULT-NOFEED.

**M3. Measure the ground offset.**

1. Start the engine. Turn the lights and the fan on.
2. Measure the DC mV between the engine block (near the sender) and the cluster ground.
3. If the value is more than 50 mV, clean and tighten the ground connections.

**M4. Measure the sender curve.**

1. Install a thermometer at the thermostat housing or the upper hose (IR thermometer or probe).
2. Disconnect the sender. Start with a cold engine.
3. While the engine becomes warm, measure the sender resistance at each 5 °C step.
4. Do the warm-up again with the sender connected, and use the ESP32 log.
5. Make a table of °C and Ω. Replace the Beta constants in the firmware with this data.

**M5. Check that the two channels agree.**

1. Disconnect the sender. Connect a 47 Ω resistor in its place.
2. Read the Rs value from the ESP32.
3. Do steps 1 and 2 again with a 100 Ω resistor.
4. If the error is more than 1%, calculate a correction factor.

**M6. Measure the system voltage.**

- Done: 12.9 V with the engine off, 14.0 V with the engine running (one 12 V battery).
- Still necessary: the minimum voltage during cranking. Use the min/max function of the multimeter.

### Phase 1: read the factory sender (built: `projects/hj60_coolant`, native tests + Wokwi)

- The ESP32 measures the feed and the sender wire as pairs.
- It keeps a pair only when the feed is "on" (> 3 V at the tap) and stable. It ignores pairs that occur during a
  contact change.
- Each second, it calculates the median k. Then it converts k → Rs → °C.
- States: `OK`, `WARN` (≥100 °C), `ALARM` (≥105 °C), `FAULT-NOFEED` (no valid pair for 5 s), `FAULT-OPEN`
  (k ≥ 0.998), `FAULT-SHORT` (Rs < 5 Ω).
- Buzzer: WARN = beep every 400 ms, ALARM = continuous, fault = two short beeps every 5 s.
- The native tests simulate a pulsing regulator, a steady feed, contact bounce and the faults.
- Wokwi uses two potentiometers to supply the divided voltages. Wokwi does not simulate resistor networks or pulses.

### Phase 2: bench, then truck

1. Bench: connect a 12 V bench supply → 25 Ω (in place of Rg) → a potentiometer or a resistor decade box (in place of
   the sender).
2. Use a relay, or switch by hand, to make pulses like the regulator.
3. Check the ESP32 reading.
4. Truck: do phase M, change the constants, and compare the ESP32 reading with a thermometer during a warm-up.

### Phase 3: from the success criteria (not built)

- **Logging**: a microSD card with an ignition sense input and a hold-up capacitor. The capacitor gives the ESP32 time
  to save the file before the power stops. Alternatively, use an NVS/flash ring buffer for summaries only, because
  flash memory accepts only a limited number of writes.
- **Changeable limits**: keep them in ESP32 `Preferences` (NVS). Set them with buttons or a WiFi page.
- **Second sensor**: a PTFE DS18B20 in an upper hose adapter. Compare it with the factory sender. Add the
  coolant-loss rate check.

## 7. Open questions

- What is Rg on this cluster (M1)? What is the real feed waveform (M2)?
- Did a previous owner install a solid-state regulator in the cluster? The firmware operates with the two types.
- What is the full sender curve (M4)? The FSM gives only two points.
- Does the gauge show a normal value now? If the needle is at the center of the scale when the engine is warm, the
  sender is probably serviceable.
- Where in the cab do we install the ESP32 and the OLED? Think about the glance angle, heat and sun.

## 8. Other connection options (B and C)

The design in sections 3 and 4 is **option A**. The sender stays connected to the gauge, and the ESP32 only listens.
This section examines two other options.

### 8.1 Option B: disconnect the gauge and measure the sender directly

**How it works:**

1. Disconnect the YG wire from gauge terminal F.
2. Connect the YG wire to the ESP32 measurement circuit.
3. The ESP32 supplies a small known current through a reference resistor (Rref) and the sender to ground.
4. The ESP32 measures the voltage above Rref (Vtop) and the voltage at the sender (Vs).
5. It calculates `Rs = Rref × Vs / (Vtop − Vs)`. This is the same equation as option A, with Rref in place of Rg.

```
 ESP32 3V3 ──[P-MOSFET switch, GPIO26]──►|── Vtop ──[ Rref 100 Ω 0.1% ]──┬── Vs ── YG wire ── sender ── ⏚ engine
                                     Schottky   │                        │
                              47k/47k + 10 nF → GPIO34     47k/47k + 10 nF + clamp → GPIO35
```

The two dividers are the same (0.1% resistors), so their gain errors cancel in the ratio.

**Numbers** (Rref = 100 Ω, about 3.0 V after the Schottky diode). The Vs column is the voltage at the sender, before
the divider:

| °C | Rs (Ω) | Vs (V) | Current (mA) |
|---|---|---|---|
| 20 | ~847 | 2.95 | 3.5 |
| 50 | 226 | 2.29 | 10.1 |
| 90 | ~55 | 1.16 | 21.4 |
| 100 | ~40 | 0.94 | 23.6 |
| 105 | ~35 | 0.85 | 24.5 |
| 115 | 26 | 0.68 | 26.2 |

- Sensitivity near 100 °C: about **19 mV for each °C** at the sender. After the 2:1 divider, this is about 9.5 mV for
  each °C at the ADC pin. Option A gives about 10.5 mV for each °C at the pin. The signal in option B is steady, so the
  average of the samples is more accurate.
- The excitation is steady. There are no regulator pulses, so the firmware does not need to ignore samples.
- The sender gets about 20–25 mW. The coolant removes this heat, so the self-heating error is very small.
- The firmware switches the excitation on for about 2 ms in each 100 ms.
- Before it switches the excitation on, it reads the sender wire with the excitation off. See "Backfeed check" below.
- **Vs must go through a divider too.** An open sender wire makes Vs equal to Vtop (about 3.0 V). Above about 2.45 V
  the ESP32 ADC compresses the reading. In the Wokwi test, an open wire then read as "8 °C, OK" instead of a fault.
  With the 2:1 dividers, both pins stay at 1.5 V or less, and an open wire gives k = 1.0.
- The ESP32 ADC has an offset of up to about 0.15 V near 0 V. An offset does not cancel in the ratio. The Wokwi ADC
  model has a similar offset and reads about 10–20 °C low at the hot end. The firmware has `ADC_OFFSET_MV` for this.
  Set it from step M5, with the same 47 Ω and 100 Ω resistors as option A.

**Protection:** the YG wire is in the engine bay. It can touch a battery wire.

- The Schottky diode stops current from a short to battery voltage before it reaches the 3.3 V supply.
- The 47 kΩ divider resistor and a clamp diode protect GPIO35.
- **Backfeed check:** before each measurement, the firmware reads the sender wire with the excitation off. If the
  voltage is more than 0.6 V in half or more of the samples in one second, it shows FAULT-BACKFEED. This finds two
  problems: terminal F is still connected to the gauge (regulator pulses), or the wire touches a battery wire.
  The ADC offset gives about 0.3 V at the wire, after the 2:1 divider. A real backfeed gives more than 3 V at the
  clamp. Thus 0.6 V separates the two.
- Measure the ground offset between the engine block and the ESP32 ground (step M3). In option B, the sender current
  also flows through this ground path.

**Effect on the truck:**

- The dash gauge **stops working**. With F open, the needle stays below C.
- If the ESP32 fails, the driver has **no coolant temperature at all**.
- Thus, use option B only on the bench, or for the M4 calibration. Do not drive the truck with option B alone.

### 8.2 Option C: measure the sender directly, then operate the gauge from the ESP32

**Important: the gauge does not accept a voltage.** The gauge is a heater between the feed (E) and terminal F. The
sender controls how much current goes from F to ground. To operate the gauge, the ESP32 must **sink a controlled
current** from F to ground. It cannot do this with a voltage output. The ESP32 DAC supplies only a few mA. The gauge
needs up to about 260 mA.

**How it works:**

1. The ESP32 measures the sender as in option B.
2. It calculates the temperature.
3. It operates a low-side MOSFET switch in series with a resistor Rmin (20 Ω) between F and ground.
4. The MOSFET switches on and off quickly (PWM, about 1 kHz). The bimetal gauge is slow, so it uses the average heat,
   as it does with the regulator pulses.
5. The ESP32 sets the PWM duty so that the gauge gets the same average heat as it would get from the real sender:
   `D = ((Rg + Rmin) / (Rg + Rs))²`.

| °C | Rs (Ω) | PWM duty D |
|---|---|---|
| 20 | ~847 | 0.3% |
| 50 | 226 | 3.2% |
| 82 | ~71 | 22% |
| 100 | ~40 | 48% |
| 105 | ~35 | 57% |
| 115 | 26 | 78% |
| 120 | ~23 | 89% |

**Parts for the gauge driver:**

- Logic-level N-channel MOSFET, rated 30 V or more.
- Rmin: 20 Ω, 5 W. The peak current is about 0.26–0.29 A. The peak power in Rmin is about 1.4–1.7 W.
- TVS diode across the MOSFET, for spikes from the regulator contacts.
- Rmin sets the hottest reading that the gauge can show. With 20 Ω, the maximum is near 125 °C.

**Fail-safe relay (necessary for option C):**

```
                  relay OFF (no power, normal closed)      relay ON (ESP32 healthy)
 gauge F ──────── sender  (factory circuit)                 MOSFET + Rmin  (ESP32 operates the gauge)
 sender  ──────── gauge F                                   ESP32 measurement (option B circuit)
```

- Use an automotive changeover relay with two poles (DPDT), or two SPDT relays.
- With no relay power, the normally closed contacts connect the sender to the gauge. This is the factory circuit.
- The ESP32 energizes the relay only when it operates correctly. Use a hardware watchdog, so that a firmware crash
  releases the relay.
- If the ESP32 fails or has no power, the relay releases. Then the dash gauge operates as the factory designed it.
- With the relay released, the ESP32 can use option A (the passive taps) to continue to measure. Thus options A and C
  can share the same hardware.

**Effect on the truck:**

- The dash gauge continues to operate. It can be more accurate, because the ESP32 can correct the sender curve.
- New risk: a firmware error can make the gauge show an incorrect temperature. Native tests and the relay fallback
  decrease this risk.
- Unknown: the regulator was designed for a steady resistive load. We did not check how it operates with a PWM load.
  **Test this on the bench** (phase 2) before you install option C.

### 8.3 Comparison

| | A: passive taps (current design) | B: direct, gauge disconnected | C: direct + ESP32 operates gauge |
|---|---|---|---|
| Dash gauge | Operates (factory) | **Does not operate** | Operates (from the ESP32) |
| If the ESP32 fails | Gauge operates | **No temperature at all** | Relay returns to the factory circuit. Gauge operates. |
| Signal at the ADC near 100 °C | ~10.5 mV/°C, pulsed | ~19 mV/°C, steady | ~19 mV/°C, steady |
| Firmware | Pair sampling, gating, median | Simple ratio | Ratio, PWM output, relay watchdog |
| Extra parts | 2 dividers | Rref, MOSFET switch, diode | Option B parts, MOSFET, Rmin, relay, watchdog |
| Changes to truck wiring | Two taps, no cut | Disconnect F | Relay in the YG wire |
| Can you go back to factory? | Remove the taps | Connect F again | Remove the relay |
| Main risk | Rg and the ADC offset limit accuracy | Driver has no gauge | PWM load on the regulator is not tested |

### 8.4 Recommendation

1. Keep **option A** for the first installation. It does not change the factory circuit.
2. Use **option B** only on the bench and for the M4 sender calibration.
3. If option A is not accurate enough after steps M4 and M5, build **option C**, with the fail-safe relay. Do the
   bench test of the regulator with a PWM load first.
4. Do not install option B alone in a truck that you drive.

Questions for options B and C:

- Does the regulator operate correctly with a PWM load (bench test)?
- Is there space for a relay near the cluster, or is the engine bay side better?
- Do you want the ESP32 to correct the dash gauge, or only to copy the sender value?

