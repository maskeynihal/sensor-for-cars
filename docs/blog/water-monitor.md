---
title: "I built a water + air monitor on an Arduino, and the buzzer tried to kill me"
subtitle: "DS18B20, DHT22, an SH1106 OLED, and one stubborn active buzzer on an UNO R4 WiFi."
tags: arduino, iot, embedded, electronics, cpp
---

I wanted one small box that watches a tank of water and the air around it. Water
too hot? Make noise. That was the whole idea. Here is how it turned out, and the
one bug that cost me an afternoon.

## What it does

An Arduino UNO R4 WiFi reads three things and shows them on a tiny screen:

- **Water temperature** from a waterproof DS18B20 probe.
- **Air temperature + humidity** from a DHT22.
- A buzzer that gets louder-in-behaviour the hotter the water gets.

All of it sits over a scrolling off-road truck animation, because a status
screen should be fun to look at.

| Water temp | Buzzer | Screen |
|---|---|---|
| below 35 °C | silent | normal |
| 35–40 °C | beep every 400 ms | flashing `WARM` |
| above 40 °C | steady on | flashing `W HOT!` |

## The parts

| Part | Job | Pin |
|---|---|---|
| UNO R4 WiFi | the brain | — |
| SH1106 OLED 128×64 | the screen | I2C: A4/A5 |
| DS18B20 probe | water temp | D2 |
| DHT22 | air temp + humidity | D4 |
| Active buzzer | the alarm | D3 |
| 4.7 kΩ resistor | DS18B20 pull-up | D2 → 5 V |

The one wire people forget is that **4.7 kΩ resistor** from the DS18B20 data
line up to 5 V. Without it the probe just reads nothing. The DHT22 has its own
pull-up built in, so it needs no extra parts.

## Both "temperature sensors" are actually digital

This surprised me. A DS18B20 and a DHT22 do not hand you a voltage you measure.
The analog sensing — a tiny bandgap circuit whose voltage drifts with heat — all
happens **inside the chip**. An on-chip ADC turns it into a number, and the chip
sends you that number over a one-wire digital protocol.

So the chain is:

```
heat → tiny voltage → on-chip ADC → digital number → one wire → your code
```

The DS18B20 gives you a signed value in **1/16 °C** steps. The library does the
division; you just read a float. That is why these sensors are so clean to use —
all the messy analog stuff is sealed away.

## The code that reads them

Reads are **non-blocking**. I never call a function that stalls the loop, because
the truck animation has to keep scrolling. Each sensor has its own timer:

```cpp
// water: fast, every 0.5 s
if (millis() - lastWater > 500) {
  float t = waterSensor.getTempCByIndex(0);
  if (t != DEVICE_DISCONNECTED_C) {
    waterC = t;
    waterF = t * 9.0 / 5.0 + 32.0;   // note the 9.0/5.0, not 9/5
    waterOK = true;
  } else waterOK = false;
  waterSensor.requestTemperatures();  // ask now, read next time
  lastWater = millis();
}
```

Two small things that matter:

1. **Read, then request.** I read last round's value and immediately ask for the
   next one. The conversion takes time; by asking ahead, the value is always
   ready and nothing blocks.
2. **`9.0 / 5.0`, not `9 / 5`.** In C++, `9 / 5` is integer division — it equals
   `1`. Your Fahrenheit would be wrong and you would stare at it for ten minutes.
   The decimals force floating-point math.

The DHT22 can only be read about twice a second, so it gets its own slower timer
and sets the pace for the serial log.

## The buzzer bug

Here is the afternoon-killer. I started with `tone()` for the beep and
`digitalWrite()` for the steady alarm, on the same pin. The buzzer jammed —
stuck on, stuck off, random.

The cause: `tone()` attaches a hardware timer to the pin and **does not let go**.
Then `digitalWrite()` fights that timer for the same pin. Two owners, one pin.

The fix was to pick one method and never mix them. This is an **active** buzzer —
it makes its own tone from plain `HIGH`, so I do not need `tone()` at all:

```cpp
if (alarm)       digitalWrite(BUZZER_PIN, HIGH);    // steady
else if (beep) { /* toggle every 400 ms with millis() */ }
else             digitalWrite(BUZZER_PIN, LOW);     // silent
```

One method for every state. Pin behaves. Lesson learned: an active buzzer and
`tone()` do not belong in the same sketch.

## What I would tell past me

- Add the DS18B20 pull-up first, before you debug "why is the water reading
  blank".
- Trust the digital sensors — do not reach for `analogRead` out of habit.
- Watch for integer division the moment you convert units.
- Pick one way to drive a pin and stick to it.

The full code, wiring diagram, and a deeper sensor explainer live in the repo.
The whole thing compiles to 32% of the UNO R4's flash, so there is plenty of
room to add WiFi logging next.
