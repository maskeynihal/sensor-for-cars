# HJ60 coolant temperature: what went wrong, what the measurements mean, and the plan

- Date: 2026-11-01
- Vehicle: Toyota Land Cruiser HJ60, 2H 4.0 L diesel, **12 V system, single battery**: 12.9 V engine off, 14.0 V
  running (owner measurement, 2026-11-01). An earlier draft said 24 V; that was wrong for this truck. Some HJ60s
  (Canada, Europe, ex-JDM) are 24 V, which is why the FSM shows a "Resister (24V)".
- Goal: read engine coolant temperature on an ESP32 DevKit V1, using the truck's **existing** gauge sender (and later our
  own probe as a cross-check). Alarm and display, then logging and configurable limits.
- Primary source: Toyota FSM, Body Electrical, pages 13-73 and 13-74 (photos in [img/](img/)).
- New to this circuit? Start with the interactive explainer: [html/circuit-explainer.html](html/circuit-explainer.html).

## 1. Summary

1. The meter readings were correct. The setup misread them. The gauge circuit is a **pulsed, regulated thermal
   (bimetal) circuit**. A multimeter can't show a steady number on it. The FSM's own test for it is "the bulb should
   start flashing".
2. 150 Ω at the sender, measured off-circuit with the engine warm, works out to about **60 °C**. That fits a truck that
   had just run.
3. The fluctuating ohms reading came from measuring the sender **while it was still connected** to the gauge. The meter
   was then measuring a network of the sender, the gauge heater, the regulator contacts and everything else on that
   feed. Ohms readings are only valid on a part that is disconnected and unpowered.
4. **The ~10 V at key-on, then fluctuation** is the gauge voltage regulator. Cold, its contacts are closed, so the
   sender node sits near the full regulated feed. Once its bimetal heats, it opens and closes over and over. The meter
   samples a square-ish wave a few times a second and shows random-looking numbers.
5. The fix is to measure the **ratio**, not the voltage. Sample the gauge feed (terminal E) and the sender wire
   (terminal F) together. The sender and the gauge heater form a resistive divider, so
   `Rs = Rg × Vs / (Vfeed − Vs)`. That holds whatever the regulator does (pulsing, battery 12 to 14.5 V, a solid-state
   regulator retrofit). The ESP32 never loads the circuit, so the dash gauge keeps working.
6. The water_monitor prototype (UNO R4 + DS18B20) can't go into this truck as is: no automotive power protection, 35/40 °C limits, a PVC
   probe cable, "probe missing = silent". See section 5.

## 2. What the FSM says (HJ60 = "BJ, HJ Series")

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

| FSM item | Value | Where |
|---|---|---|
| Gauge type | Bimetal (thermal) receiver | 13-74 "Inspection (Bimetal Type)" |
| Gauge heater resistance E–F | **STD 25 Ω** | 13-74 |
| Regulator | Built into the fuel receiver gauge | 13-74 |
| Feed on 24 V trucks | Through a series "Resister (24V)" after the 15 A ENGINE fuse. **This truck is 12 V**, so expect no resistor; check in M0 | 13-73 BJ/HJ diagram |
| Feed at terminal A | "2–7 V" with the ignition on (wording is ambiguous; see note) | 13-74 |
| Receiver test | Ground the sender wire through a 3.4 W bulb. The bulb **flashes** and the needle **vibrates** | 13-73 |
| Sender | 50 °C → 226 Ω, 115 °C → 26 Ω | 13-74 table |

The "2–7 V at terminal A" line doesn't sit cleanly with the diagram, where A is the regulator *input*. It probably
means the averaged reading a meter shows. The ratio method doesn't depend on it, so it's left unresolved here.

Other HJ60 facts (vendor/forum, lower confidence than the FSM):

- Thermostat 90916-03026 starts to open at 82 °C and is fully open at 95 °C
  ([allfourx4](https://www.allfourx4.com.au/Thermostat-suitable-for-Landcruiser-H-2H-3B-HJ-BJ-40-60-75-Genuine-90916-03026)).
- Radiator cap 0.9 kg/cm² (about 88 kPa, 13 psi)
  ([allfourx4/Tridon](https://www.allfourx4.com.au/Radiator-Cap-suitable-for-Prado-Landcruiser-Hilux-Surf-4Runner-Patrol-Navara-Tridon-CB1390/)).
  Under that cap 50/50 glycol boils at about 124–126 °C, and plain water at about 118 °C (calculated).
- Normal running is about 85–88 °C ([ih8mud](https://forum.ih8mud.com/threads/oem-temperature-gauge-fault.1385755/)).
  2H heads crack after overheating ([ih8mud](https://forum.ih8mud.com/threads/a-2h-story.757492/)).
- Sender thread is M16×1.5, single terminal, on the head. Cross-references 83420-20011 / 83420-20020 / 83420-16010
  ([euro4x4parts](https://www.euro4x4parts.com/en/parts/ref=KSD1008/sender-coolant-temperature/oe=83420-20011_mc850070_82180-4150_83420-20020_21203-aa000_83420-16010)).
- **Don't confuse it** with the 2-pin glow-plug-control sensor 89422-30020 near the thermostat housing (about 2 kΩ at
  21 °C) ([allfourx4](https://www.allfourx4.com.au/Oil-Temperature-Sensor-suitable-for-Landcruiser-2H-12HT-Diesel-89422-30020A),
  [ih8mud](https://forum.ih8mud.com/threads/2h-wiring-question.83647)).
- No ECU, no OBD. The 2H has a mechanical injection pump, so the gauge sender is the only factory coolant signal.

## 3. What the measurements mean

### 3.1 Sender sender-to-ground, disconnected: 150 Ω

The FSM gives two points. A Beta (NTC) fit through them gives B ≈ 4173 K. **This is an approximation**: two points
65 °C apart, extrapolated. It has to be calibrated (section 6, step M4).

| °C | Rs (Ω) | k = Rs/(Rs+25) |
|---|---|---|
| 20 | ~847 | 0.971 |
| 50 | 226 (FSM) | 0.900 |
| **60** | **~153** | 0.860 |
| 82 (thermostat opens) | ~71 | 0.738 |
| 90 | ~55 | 0.686 |
| 100 | ~40 | 0.616 |
| 105 | ~35 | 0.580 |
| 110 | ~30 | 0.545 |
| 115 | 26 (FSM) | 0.510 |
| 120 | ~23 | 0.476 |

150 Ω → **about 60 °C**. That's consistent with "warm, just ran". The sender is probably healthy. A cold engine at 20 °C
should read around 850 Ω; check that too (M4).

**Error bar: large until M4.** A forum post for the FJ60 (petrol) sender quotes 27 Ω at 100 °C and 23 Ω at 105 °C
([ih8mud](https://forum.ih8mud.com/threads/how-i-fixed-my-temp-gauge.1014115/)). On this Beta fit those resistances
read about 114 °C and 119 °C, so the two sources disagree by about 14 °C right at the WARN/ALARM thresholds. Different
engine and sender, but it shows how loose a two-point fit is. **Until the M4 calibration, readings near 100 °C can be
off by more than ±10 °C. Don't trust WARN/ALARM yet.**

### 3.2 Ohms with the gauge connected: fluctuating

A multimeter on ohms pushes a small current and measures the voltage. Connected to the gauge, the sender is in
parallel with a path through the 25 Ω gauge heater, the regulator contacts, and the fuel gauge circuit. That path
changes as the bimetal contacts move and cool. With the key on, the circuit's own voltage also swamps the meter. The
reading is meaningless, not a fault. **Rule: resistance only with the sender wire unplugged and the key off.**

### 3.3 Volts at the spliced sender wire: 0 V key off, ~10 V for a few seconds, then wild

- Key off: no feed, 0 V. Correct.
- Key on: the regulator's bimetal is cold, so its contacts are closed. The feed sits at its "on" level, and the sender
  node is `Vfeed × Rs/(Rs+25)`. With Rs ≈ 150 Ω, k = 0.86, so 10 V at the sender means about **11.7 V of feed** during
  the on-time. That's the 12.9 V battery minus drops in the ignition relay, fuse, wiring and regulator contacts. With
  the engine running (14 V) expect the on-level to rise to about 12.5–13 V. The ratio method doesn't care.
- After a few seconds the regulator heater warms its bimetal, the contacts open (node → ~0 V), it cools, they close
  again. The feed becomes a pulse train. Its average sets the needle, because the gauge is also a slow heater that
  averages. This is exactly the FSM's bulb-flashing test.
- A typical DMM takes 2 to 5 readings per second and isn't synchronised with the pulses. Each reading lands at a
  different point of the on/off cycle, so the display jumps. **A pulse waveform can't be read with a DC meter.** You'd
  need a scope, or the ESP32 capture sketch in M2.

### 3.4 Why the ratio method works

The gauge heater (E–F) and the sender form a voltage divider fed by the regulator:

```
Vfeed (E) ──[ Rg = 25 Ω ]── F = Vs ──[ Rs ]── ⏚
k = Vs / Vfeed = Rs / (Rs + Rg)     ⇒     Rs = Rg × k / (1 − k)
```

- The divider is resistive and responds instantly, so k is the same at every instant the feed is on. It doesn't matter
  whether the feed is 7 V, 11.7 V or 14 V, or how it pulses.
- When the feed is off, both are ~0 V, and those samples are thrown away.
- Rg changes the answer directly (1 Ω of error in Rg is 4% of Rs). **Measure E–F on this truck's cluster (M1). Don't
  trust "25 Ω STD" blindly.**
- Ground offset: the sender grounds at the engine block and the ESP32 at the cabin. A few tens of mV between them adds
  about 0.5 °C of error. Ground the ESP32 at the cluster and measure the offset (M3).

## 4. How to connect the ESP32 (DevKit V1)

### 4.1 Signal taps: two high-impedance dividers, one per channel

Tap the two ends of the temperature gauge's heater: **E** (regulated feed) and **F** / the YG wire (sender). Use
identical networks.

**Find E and F first (M0).** The FSM labels the cluster connector pins D/B/A/13/1/G (Fig 13-148), not E/F, and the
regulator sits inside the fuel gauge. The regulated feed may not come out on the connector, so you may have to open
the cluster and tap at the gauge's own terminals.

- **Don't use connector pin A as the feed.** A is the regulator *input*. It stays high while the regulator is off and
  the sender drops to ~0 V. The firmware would accept those pairs as valid with k ≈ 0 and report FAULT-SHORT or a
  false hot reading.
- Check: key off, cluster unplugged, the two tap points read **about 25 Ω** between them (that's the gauge heater).
- Check: key on, the feed tap never reads lower than the sender tap.

```
 tap ──[ 68 kΩ 0.1% ]───┬───────────┬──────► ESP32 ADC1 pin (GPIO34 = feed, GPIO35 = sender)
                        │           │
                     [10 kΩ 0.1%] [100 nF]        BAT54S: clamps to 3V3 and GND at the ADC pin
                        │           │
                       ⏚ (cluster ground = ESP32 GND)
```

- Ratio 7.8:1, sized for the 12 V system. A 16 V worst case (a failed alternator regulator overcharging) becomes 2.05 V,
  inside the ESP32 accurate band at 11 dB attenuation (about 0.15 to 2.45 V). The measured on-level of ~11.7 V
  becomes ~1.5 V. Spikes above that are the clamp's job.
- Sensitivity near 100 °C: k changes by about 0.007 per °C, about 10.5 mV per °C at the ADC with an 11.7 V feed.
  Averaging 64 to 128 samples gets this to about 0.2 to 0.3 °C.
- Load on the gauge circuit: 78 kΩ against a 25 to 200 Ω node, below 0.3%. The needle doesn't move.
- The 100 nF capacitor gives τ ≈ 0.9 ms. That's an anti-alias filter for contact bounce, and much faster than the
  regulator cycle (seconds).
- Use matched (0.1%) resistors on both channels, because the divider ratio cancels in k. Or calibrate with a fixed
  resistor in place of the sender (M5).
- **Never** put a low-value part on the tap. The FSM uses a 3.4 W bulb rather than a dead short for a reason: a short
  cooks the gauge heater. Same rule for the probes: no ohms with power on.
- Use ADC1 pins only (GPIO32 to 39). ADC2 is unavailable while WiFi is on.
- **ESP32 ADC offset is the biggest error source.** Even with eFuse calibration (`analogReadMilliVolts`), expect
  roughly ±20–30 mV. At a 1.5 V feed that is up to about 2% in k, or 2–3 °C. Calibrate with fixed resistors (M5) and
  add an offset constant if needed.

### 4.2 Power: 12 V truck → 5 V ESP32

The ESP32 DevKit V1 takes 5 V on its VIN/5V pin. **Don't** feed it 12 V. Build the front end in this order
(details in [ADR 0009](../../../docs/adr/0009-hj60-12v-power-front-end.md), Proposed):

1. **Fuse** at the tap (1 A), on an ignition-switched circuit.
2. **Reverse-polarity protection**: a Schottky diode, or an ideal-diode controller.
3. **TVS diode** across the input. Standoff about 26–28 V, so a 24–26 V jump start doesn't make it conduct. Its clamp
   voltage (about 45 V) must sit below the converter's maximum input.
4. **Buck converter rated for at least 60 V input** (40 V is the bare minimum), with a 5 V output, feeding the
   DevKit's 5V pin. It should hold 5 V down to about 6 V input. If the diesel's cranking dip resets the ESP32, that's
   acceptable: it reboots in about a second into `WAIT`.
5. **Common ground at the cluster.**

12 V figures (vendor app notes quoting the standards):

- ISO 16750-2 load dump, Test A (unsuppressed): 79–101 V for 40–400 ms
  ([TI SNOAAA1](https://ti.com/document-viewer/lit/html/SNOAAA1)).
- Test B (clamped): 35 V
  ([ADI](https://www.analog.com/en/resources/technical-articles/low-quiescent-current-surge-stopper-robust-automotive-supply-protection.html)).
- Cranking dips to about 6 V
  ([MPS](https://media.monolithicpower.com/mps_cms_document/c/o/cold_crank_to_load_dump-primer_ver1.3.pdf)).
- Jump start: 24–26 V for 60 s. Reverse battery: −14 V for 60 s
  ([ATEC summary](https://www.atecorp.com/compliance-standards/iso/ISO-16750-2)).
- ISO 7637-2 12 V pulses: 1 = −75/−100 V, 3b = +75/+150 V
([Littelfuse](https://www.mouser.com/pdfdocs/Littelfuse_TVS_Diode_Automotive_Circuit_Protection_Using_Automotive_TVS_Diodes_Application_Notepdf.pdf)).

### 4.3 Outputs

- 1.3" SH1106 OLED on I2C (GPIO21 SDA, GPIO22 SCL). Static readout, no animation (section 5).
- Active buzzer on GPIO25 **through an NPN transistor** (e.g. 2N2222 / S8050 with 1 kΩ base resistor and a flyback
  diode). A 5 V active buzzer draws more current than an ESP32 pin should source.

## 5. What we did wrong in water_monitor (for this use)

water_monitor was a bench project that watched "a tank of water". As an HJ60 coolant monitor it gets these things wrong:

| # | Problem | Why it matters in the HJ60 | Fix |
|---|---|---|---|
| 1 | Thresholds 35 / 40 °C | The coolant is normally 82–95 °C, so it would alarm every drive | Warn ≥100 °C, alarm ≥105 °C, with 2 °C hysteresis. Make them configurable (phase 3) |
| 2 | Missing probe = silent | A failed sensor looks like "all fine". The alarm fails silently | Fail loud: sender open, short, or no feed is a **fault**, with its own sound and screen |
| 3 | DS18B20 PVC "waterproof" probe | PVC cable is rated to about 80–85 °C. The chip maxes out at 125 °C, at the edge of the 124 °C pressurised boil point. ±0.5 °C only from −10 to +85 °C | Use the OEM sender as primary. For a second sensor, PTFE-cable DS18B20 in a hose-adapter thermowell, not bare in pressurised glycol |
| 4 | DS18B20 power-on value 85.0 °C | 85.0 is a normal coolant reading, so a reset glitch looks valid | Reject exactly 85.0 right after a conversion error (if the DS18B20 is used) |
| 5 | UNO R4 on USB, or VIN with no transient protection | VIN (6–24 V) only has Schottky diodes: no TVS. A 12 V truck sees 35–101 V load dumps and cranking dips to about 6 V | Automotive front end feeding 5 V (4.2) |
| 6 | No transient, reverse or fuse protection | Load dump, jump start, cranking | 4.2 |
| 7 | Breadboard + Dupont jumpers | Vibration from 10 to 2000 Hz fretts low-force contacts | Soldered perfboard or PCB, locking connectors, strain relief |
| 8 | Animated truck scene | It pulls the driver's eyes. NHTSA guidance is 2 s per glance and 12 s per task. OLED life is about 10k h at full brightness | Static big number, dim, blank when idle, flash only on alarm |
| 9 | DHT22 on the dash | A parked dash in sun can pass its 80 °C rating | Drop it, or mount it shaded (not needed for coolant) |
| 10 | Only a voltmeter / ohmmeter to diagnose | Pulsed circuit (3.3) | ESP32 capture sketch (M2), ratio method |
| 11 | Measuring ohms in circuit | Meaningless reading (3.2) | Disconnect and key off |
| 12 | `display.begin()` failure halts the sketch (`for(;;)`) | A loose OLED connector would also kill the alarm | Log it and keep running (done in hj60_coolant) |

A gap that remains even with the new design: **coolant loss**. If the coolant drops below the sender, it sits in steam
or air and reads *cooler*, and the gauge (and our ESP32) can show a falling temperature while the engine overheats.
Mitigations (next steps, not built): a rate-of-change check, a second sensor lower in the circuit or on the upper hose,
or a coolant-level probe.

## 6. Plan

### Phase M: measurements only you can do on the truck (before trusting numbers)

| Step | What | How | Record |
|---|---|---|---|
| M0 | Find the tap points | Key off, cluster out. Find two points with about 25 Ω between them: the temp gauge's E and F (may be inside the cluster). **Not connector pin A**. Key on: feed point ≥ sender point at all times | Where E and F are; photo |
| M1 | Gauge heater Rg | Key off. Unplug the cluster. Ohms across the temp gauge's E and F | Rg (Ω) → `GAUGE_R_OHMS` |
| M2 | Feed waveform | Build the two dividers (4.1). Flash `projects/_utilities/esp32_gauge_capture`. Key on, capture 30 s cold, and 30 s warm with the engine running | On-level V, period, duty, bounce. Set `FEED_ON_V`; if on-pulses are short (under ~50 ms per second), lower `SAMPLE_MS` / `MIN_PAIRS` or the firmware will raise a false FAULT-NOFEED |
| M3 | Ground offset | Engine running, lights and fan on. DC mV between the engine block (by the sender) and cluster ground | mV. Above 50 mV, fix the grounds |
| M4 | Sender curve | Thermometer at the thermostat housing or upper hose (IR gun or a probe), sender disconnected, warm-up from cold. Ohms every 5 °C. Repeat connected using the ESP32 log | A table of °C vs Ω → replace the Beta constants |
| M5 | Channel match | Replace the sender with a 47 Ω and a 100 Ω resistor. Check the ESP32 Rs reading | Correction factor if off by more than 1% |
| M6 | System voltage | Done: 12.9 V engine off, 14.0 V running (single 12 V battery). Still useful: the minimum during cranking (DMM min/max mode) | Cranking minimum V |

### Phase 1: read the OEM sender (built: `projects/hj60_coolant`, native tests + Wokwi)

- ESP32 samples feed and sender as pairs, keeps pairs where the feed is "on" (> 3 V at the tap) and stable (rejects
  pairs straddling a contact edge), takes the median k over 1 s, and converts k → Rs → °C.
- States: `OK`, `WARN` (≥100 °C), `ALARM` (≥105 °C), `FAULT-NOFEED` (no valid pair for 5 s), `FAULT-OPEN` (k ≥ 0.998),
  `FAULT-SHORT` (Rs < 5 Ω).
- Buzzer: warn = beep 400 ms, alarm = steady, fault = double chirp every 5 s.
- Native tests model a pulsing regulator, a steady feed, contact bounce, and the faults. Wokwi models the
  post-divider voltages with two potentiometers (Wokwi doesn't simulate resistor networks or pulses).

### Phase 2: bench then truck

1. Bench: a 12 V bench supply → 25 Ω (Rg stand-in) → a pot or resistor decade as the sender. Pulse it with a relay or
   by hand to mimic the regulator. Check the ESP32 reading.
2. Truck: complete phase M, update the constants, compare against a thermometer through a warm-up.

### Phase 3: from the success criteria (not built)

- **Logging**: microSD with an ignition-sense input and a hold-up capacitor, so the file is flushed before power dies.
  Or NVS/flash ring buffer for summaries only (flash wear).
- **Configurable limits**: stored in ESP32 `Preferences` (NVS), set with buttons or a WiFi page.
- **Second sensor**: PTFE DS18B20 in an upper-hose adapter, cross-checked against the OEM sender, plus the
  coolant-loss rate check.

## 7. Open questions

- Rg on this cluster (M1) and the real feed waveform (M2).
- Has the cluster had a solid-state regulator retrofit? The firmware handles both.
- The full sender curve (M4); the FSM gives only two points.
- Is the gauge reading normal today? If the needle sits mid-scale warm, the sender is probably fine.
- Where to mount the ESP32 and OLED in the cab (glance angle, heat, sun).
