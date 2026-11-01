// Shared truck models and helpers for the hj60_coolant native tests.
#pragma once
#include "hwtest.h"
#include "DIYables_TFT_SPI.h"

float ohmsToC(float rs);  // from the sketch
enum SenderMode { SENDER_MODE_A, SENDER_MODE_B };
extern SenderMode senderMode;

const int FEED = 34, SENDER = 35, BUZZER = 25, EXCITE = 26, BATT = 32, BUTTON = 33;
const double RG = 25.0, RATIO = 7.8, RREF = 100.0;

// Sender resistance for a temperature, same Beta fit as the sketch
// (226 ohm @ 50 C, 26 ohm @ 115 C).
inline double ohmsFor(double c) {
  const double t0 = 323.15, t1 = 388.15;
  const double b = std::log(226.0 / 26.0) / (1 / t0 - 1 / t1);
  return 226.0 * std::exp(b * (1 / (c + 273.15) - 1 / t0));
}

// ADC millivolts for a voltage seen through a divider.
inline int mvAt(double volts, double ratio) { return (int)std::lround(volts / ratio * 1000.0); }
inline int mv(double tapVolts) { return mvAt(tapVolts, RATIO); }

// Mode A: regulator feed (pulsed or steady) across the 25 ohm gauge heater
// in series with the sender. Taps through 68k/10k.
struct Truck {
  double rs = 54.5;          // sender ohms
  double feedOn = 11.7;      // feed level while the regulator contacts are closed
  unsigned long onMs = 0;    // 0 = steady feed (solid-state regulator)
  unsigned long periodMs = 0;
  int bounceEvery = 0;       // every Nth sender read returns 0 V (contact bounce)
  int senderReads = 0;
  double batt = 12.9;

  bool feedIsOn() const { return onMs == 0 || (sim::now % periodMs) < onMs; }
  int read(uint8_t pin) {
    double v = feedIsOn() ? feedOn : 0.0;
    if (pin == FEED) return mv(v);
    if (pin == SENDER) {
      senderReads++;
      if (bounceEvery && senderReads % bounceEvery == 0) return 0;
      return mv(std::isinf(rs) ? v : v * rs / (rs + RG));
    }
    if (pin == BATT) return mv(batt);
    return 0;
  }
};

// Mode B: the YG wire goes to the ESP32. GPIO26 switches 3.3 V through a
// Schottky (Vtop ~3.0 V) and Rref 100 ohm into the sender. Vtop and Vs are
// each read through a 47k/47k divider. backfeedV puts a voltage on the sender wire
// (gauge still connected, or a short to a battery wire), clamped at 3.3 V.
struct DirectTruck {
  double rs = 54.5;
  double vtop = 3.0;
  bool exciteDead = false;
  double backfeedV = 0;      // volts on the wire from outside
  unsigned long backOnMs = 0, backPeriodMs = 0;  // 0 = constant backfeed
  double batt = 12.9;

  bool exciting() const { return sim::pinLevel[EXCITE] == HIGH && !exciteDead; }
  double outside() const {
    if (backfeedV <= 0) return 0;
    if (backOnMs == 0 || (sim::now % backPeriodMs) < backOnMs) return backfeedV;
    return 0;
  }
  int read(uint8_t pin) {
    double vt = exciting() ? vtop : 0.0;
    if (pin == FEED) return mvAt(vt, 2.0);
    if (pin == SENDER) {
      double v = std::isinf(rs) ? vt : vt * rs / (rs + RREF);
      v = std::max(v, std::min(3.3, outside()));
      return mvAt(v, 2.0);
    }
    if (pin == BATT) return mv(batt);
    return 0;
  }
};

inline Truck truck;
inline DirectTruck direct;

inline void attach() {
  senderMode = SENDER_MODE_A;
  sim::analogMvFn = [](uint8_t pin) { return truck.read(pin); };
}

inline void attachB() {
  senderMode = SENDER_MODE_B;
  sim::analogMvFn = [](uint8_t pin) { return direct.read(pin); };
}

// Value after the last "Coolant: " in the serial log, NAN if "--".
inline double lastCoolant() {
  size_t at = sim::serialOut.rfind("Coolant: ");
  if (at == std::string::npos) return NAN;
  const char *p = sim::serialOut.c_str() + at + 9;
  if (p[0] == '-') return NAN;
  return std::atof(p);
}

inline std::string lastLine() {
  size_t end = sim::serialOut.rfind("\r\n");
  if (end == std::string::npos) return "";
  size_t start = sim::serialOut.rfind("\r\n", end - 1);
  size_t from = start == std::string::npos ? 0 : start + 2;
  return sim::serialOut.substr(from, end - from);
}

#define CHECK_NEAR(a, b, tol)                                                                  \
  do {                                                                                         \
    double _a = (a), _b = (b);                                                                 \
    if (!(std::fabs(_a - _b) <= (tol)))                                                        \
      hwtest::fail(__FILE__, __LINE__, "CHECK_NEAR got " + std::to_string(_a) + " vs " +      \
                                           std::to_string(_b));                                \
  } while (0)

inline void runAt(double c, unsigned long ms = 3000) {
  truck.rs = ohmsFor(c);
  direct.rs = ohmsFor(c);
  sim::runFor(ms);
}

// Press and release the screen button.
inline void pressButton() {
  sim::pinLevel[BUTTON] = LOW;
  sim::runFor(100);
  sim::pinLevel[BUTTON] = HIGH;
  sim::runFor(600);
}
