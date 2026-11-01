#include "Arduino.h"
#include "Wire.h"
#include "SPI.h"

namespace sim {

unsigned long now = 0;
uint8_t pinLevel[kPins];
uint8_t pinModeOf[kPins];
int analogValue[kPins];
int analogMv[kPins];
std::function<int(uint8_t)> analogMvFn;
std::vector<PinEvent> pinLog;
std::string serialOut;
std::string serialIn;
std::vector<uint8_t> i2cDevices;
bool displayBeginOk = true;
std::string frameText;
std::string lastFrameText;
unsigned long framesShown = 0;
float ds18b20C[8];
int ds18b20Count = 1;
float dhtTempC = NAN;
float dhtHumidity = NAN;

void reset() {
  now = 0;
  for (int i = 0; i < kPins; i++) { pinLevel[i] = LOW; pinModeOf[i] = INPUT; analogValue[i] = 0; analogMv[i] = 0; }
  analogMvFn = nullptr;
  pinLog.clear();
  serialOut.clear();
  serialIn.clear();
  i2cDevices.clear();
  displayBeginOk = true;
  frameText.clear();
  lastFrameText.clear();
  framesShown = 0;
  for (float &t : ds18b20C) t = NAN;
  ds18b20Count = 1;
  dhtTempC = NAN;
  dhtHumidity = NAN;
}

void runFor(unsigned long ms) {
  unsigned long end = now + ms;
  while (now < end) {
    unsigned long before = now;
    loop();
    if (now == before) now += 1;
  }
}

int togglesBetween(uint8_t pin, unsigned long from, unsigned long to) {
  int n = 0;
  for (const PinEvent &e : pinLog)
    if (e.pin == pin && e.t >= from && e.t < to) n++;
  return n;
}

}  // namespace sim

// ---------------- core ----------------
long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void pinMode(uint8_t pin, uint8_t mode) {
  if (pin >= sim::kPins) return;
  sim::pinModeOf[pin] = mode;
  if (mode == INPUT_PULLUP) sim::pinLevel[pin] = HIGH;  // idle level of a pulled-up input
}

void digitalWrite(uint8_t pin, uint8_t val) {
  if (pin >= sim::kPins) return;
  val = val ? HIGH : LOW;
  if (sim::pinLevel[pin] != val) sim::pinLog.push_back({sim::now, pin, val});
  sim::pinLevel[pin] = val;
}

int digitalRead(uint8_t pin) { return pin < sim::kPins ? sim::pinLevel[pin] : LOW; }
int analogRead(uint8_t pin) { return pin < sim::kPins ? sim::analogValue[pin] : 0; }
uint32_t analogReadMilliVolts(uint8_t pin) {
  if (pin >= sim::kPins) return 0;
  int mv = sim::analogMvFn ? sim::analogMvFn(pin) : sim::analogMv[pin];
  return mv < 0 ? 0 : (uint32_t)mv;
}
void analogWrite(uint8_t pin, int val) { digitalWrite(pin, val > 127 ? HIGH : LOW); }

unsigned long millis() { return sim::now; }
unsigned long micros() { return sim::now * 1000UL; }
void delay(unsigned long ms) { sim::now += ms; }
void delayMicroseconds(unsigned int) {}

long random(long max) { return max > 0 ? std::rand() % max : 0; }
long random(long min, long max) { return max > min ? min + std::rand() % (max - min) : min; }
void randomSeed(unsigned long seed) { std::srand((unsigned)seed); }

// ---------------- Print ----------------
size_t Print::print(long v, int base) {
  if (base == DEC) { char b[24]; snprintf(b, sizeof b, "%ld", v); return write(b); }
  return print((unsigned long)v, base);
}

size_t Print::print(unsigned long v, int base) {
  if (base < 2) base = DEC;
  char b[66]; int i = 64; b[65] = 0;
  do { int d = (int)(v % base); b[i--] = (char)(d < 10 ? '0' + d : 'A' + d - 10); v /= base; } while (v && i >= 0);
  return write(&b[i + 1]);
}

size_t Print::print(double v, int digits) {
  if (std::isnan(v)) return write("nan");
  if (std::isinf(v)) return write("inf");
  char b[48]; snprintf(b, sizeof b, "%.*f", digits, v);
  return write(b);
}

int HardwareSerial::read() {
  if (sim::serialIn.empty()) return -1;
  int c = (uint8_t)sim::serialIn[0];
  sim::serialIn.erase(0, 1);
  return c;
}

HardwareSerial Serial;

// ---------------- Wire / SPI ----------------
uint8_t TwoWire::endTransmission(bool) {
  for (uint8_t a : sim::i2cDevices) if (a == txAddr_) return 0;  // ACK
  return 2;                                                       // NACK on address
}

TwoWire Wire;
SPIClass SPI;
