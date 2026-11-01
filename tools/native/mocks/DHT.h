#pragma once
#include "Arduino.h"

#define DHT11 11
#define DHT12 12
#define DHT21 21
#define DHT22 22
#define AM2301 21

class DHT {
public:
  DHT(uint8_t, uint8_t, uint8_t = 6) {}
  void begin(uint8_t = 55) {}
  float readTemperature(bool fahrenheit = false, bool = false) {
    if (std::isnan(sim::dhtTempC)) return NAN;
    return fahrenheit ? sim::dhtTempC * 9.0f / 5.0f + 32.0f : sim::dhtTempC;
  }
  float readHumidity(bool = false) { return sim::dhtHumidity; }
};
