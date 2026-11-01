// DS18B20 mock. Like the real chip, a reading is latched at
// requestTemperatures() and returned by the next getTemp*() call.
#pragma once
#include "OneWire.h"

#define DEVICE_DISCONNECTED_C -127
#define DEVICE_DISCONNECTED_F -196.6

class DallasTemperature {
public:
  explicit DallasTemperature(OneWire *) { for (float &t : latched_) t = DEVICE_DISCONNECTED_C; }
  void begin() {}
  void setResolution(uint8_t) {}
  void setWaitForConversion(bool) {}
  uint8_t getDeviceCount() { return (uint8_t)sim::ds18b20Count; }

  void requestTemperatures() {
    for (int i = 0; i < 8; i++)
      latched_[i] = (i < sim::ds18b20Count && !std::isnan(sim::ds18b20C[i])) ? sim::ds18b20C[i] : DEVICE_DISCONNECTED_C;
  }
  float getTempCByIndex(uint8_t i) { return i < 8 ? latched_[i] : DEVICE_DISCONNECTED_C; }
  float getTempFByIndex(uint8_t i) {
    float c = getTempCByIndex(i);
    return c == DEVICE_DISCONNECTED_C ? DEVICE_DISCONNECTED_F : c * 9.0f / 5.0f + 32.0f;
  }

private:
  float latched_[8];
};
