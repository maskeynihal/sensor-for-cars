// =====================================================================
//  Native sim state -- the "fake world" the mocks read from / write to.
// ---------------------------------------------------------------------
//  Tests set inputs (sensor values, I2C devices) and read outputs (pin
//  levels, serial text, OLED text) through this namespace. Time is fake:
//  millis() returns sim::now, delay() advances it.
// =====================================================================
#pragma once
#include <cmath>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace sim {

struct PinEvent {
  unsigned long t;
  uint8_t pin;
  uint8_t value;
};

constexpr int kPins = 64;

// ---- time ----
extern unsigned long now;  // ms

// ---- GPIO ----
extern uint8_t pinLevel[kPins];
extern uint8_t pinModeOf[kPins];
extern int analogValue[kPins];
// ESP32 analogReadMilliVolts(): returns analogMvFn(pin) if set (time-varying
// inputs, e.g. a pulsing gauge regulator; read sim::now inside), else analogMv[pin].
extern int analogMv[kPins];
extern std::function<int(uint8_t pin)> analogMvFn;
extern std::vector<PinEvent> pinLog;  // every digitalWrite that changed a level

// ---- Serial ----
extern std::string serialOut;  // everything printed since reset()
extern std::string serialIn;   // bytes waiting to be read

// ---- I2C ----
extern std::vector<uint8_t> i2cDevices;  // addresses that ACK

// ---- OLED (any Adafruit_GFX display) ----
extern bool displayBeginOk;
extern std::string frameText;     // text drawn since last clearDisplay()
extern std::string lastFrameText; // text of the last frame pushed by display()
extern unsigned long framesShown;

// ---- DS18B20 (DallasTemperature) ----
// NAN == probe unplugged (library returns DEVICE_DISCONNECTED_C).
extern float ds18b20C[8];
extern int ds18b20Count;

// ---- DHT11/22 ----
// NAN == read failure.
extern float dhtTempC;
extern float dhtHumidity;

void reset();

// Run the sketch's loop() until `ms` of fake time has passed.
// If one loop() call does not advance time, 1 ms is added (models CPU time).
void runFor(unsigned long ms);

// Count digitalWrite level changes on `pin` within [from, to).
int togglesBetween(uint8_t pin, unsigned long from, unsigned long to);

}  // namespace sim
