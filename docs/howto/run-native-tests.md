# Run native (host) tests

```sh
make test P=water_monitor              # both copies (sim + firmware)
make test-sim P=water_monitor          # sim copy only
make test P=water_monitor T=threshold  # tests whose name contains "threshold"
```

## How it works

`scripts/test.sh` runs:

```
clang++ tools/native/mocks/arduino_mock.cpp  projects/<p>/emulator/native/test_*.cpp \
        -x c++ -include Arduino.h  <the .ino, unmodified>
```

- `tools/native/mocks/` provides `Arduino.h`, `Wire.h`, `SPI.h`, `Adafruit_GFX.h`, `Adafruit_SH110X.h`,
  `Adafruit_SSD1306.h`, `OneWire.h`, `DallasTemperature.h`, `DHT.h`.
- `sim.h` is the fake world:

| Set (inputs) | Read (outputs) |
|---|---|
| `sim::ds18b20C[i]` (NAN = unplugged) | `sim::pinLevel[pin]` |
| `sim::dhtTempC`, `sim::dhtHumidity` (NAN = fail) | `sim::pinLog`, `sim::togglesBetween(pin, t0, t1)` |
| `sim::i2cDevices` (ACKing addresses) | `sim::serialOut` |
| `sim::analogValue[pin]` | `sim::lastFrameText` (text on the last OLED frame) |
| `sim::displayBeginOk` | `sim::now` (fake ms) |

- `delay()` advances `sim::now`. `sim::runFor(ms)` calls `loop()` until that much fake time has passed.
- Each `TEST` runs in a forked child, so globals and statics are fresh. A hang is killed after 5 s.

## Writing a test

```cpp
#include "hwtest.h"
TEST(alarm_above_40) {
  sim::ds18b20C[0] = 41; sim::dhtTempC = 22; sim::dhtHumidity = 50;
  setup();
  sim::runFor(3000);
  CHECK_EQ(sim::pinLevel[3], HIGH);
  CHECK_CONTAINS(sim::serialOut, "Buzzer: LOUD-CONT");
}
```

Then break the code on purpose and confirm the test fails (mutation check).

## Limits

The mocks don't run the real libraries, I2C timing, or pixel output. Wokwi covers those. A new library needs a
mock. Keep it minimal and behaviourally honest.
