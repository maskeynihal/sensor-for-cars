// Native tests for hj60_coolant. Compiled against the real .ino with the
// mocks in tools/native/mocks. Run: make test P=hj60_coolant
//
// The truck side is modelled as the FSM circuit: a regulator feed V (pulsed
// or steady) across the 25 ohm gauge heater in series with the sender Rs.
// The ADC sees each tap through the 7.8:1 divider (68k/10k).
#include "hwtest.h"

float ohmsToC(float rs);  // from the sketch

namespace {

const int FEED = 34, SENDER = 35, BUZZER = 25;
const double RG = 25.0, RATIO = 7.8;

// Sender resistance for a temperature, same Beta fit as the sketch
// (226 ohm @ 50 C, 26 ohm @ 115 C).
double ohmsFor(double c) {
  const double t0 = 323.15, t1 = 388.15;
  const double b = std::log(226.0 / 26.0) / (1 / t0 - 1 / t1);
  return 226.0 * std::exp(b * (1 / (c + 273.15) - 1 / t0));
}

int mv(double tapVolts) { return (int)std::lround(tapVolts / RATIO * 1000.0); }

struct Truck {
  double rs = 54.5;          // sender ohms
  double feedOn = 11.7;      // feed level while the regulator contacts are closed
  unsigned long onMs = 0;    // 0 = steady feed (solid-state regulator)
  unsigned long periodMs = 0;
  int bounceEvery = 0;       // every Nth sender read returns 0 V (contact bounce)
  int senderReads = 0;

  bool feedIsOn() const { return onMs == 0 || (sim::now % periodMs) < onMs; }
  int read(uint8_t pin) {
    double v = feedIsOn() ? feedOn : 0.0;
    if (pin == FEED) return mv(v);
    if (pin == SENDER) {
      senderReads++;
      if (bounceEvery && senderReads % bounceEvery == 0) return 0;
      return mv(std::isinf(rs) ? v : v * rs / (rs + RG));
    }
    return 0;
  }
};

Truck truck;

void attach() {
  sim::analogMvFn = [](uint8_t pin) { return truck.read(pin); };
}

// Value after the last "Coolant: " in the serial log, NAN if "--".
double lastCoolant() {
  size_t at = sim::serialOut.rfind("Coolant: ");
  if (at == std::string::npos) return NAN;
  const char *p = sim::serialOut.c_str() + at + 9;
  if (p[0] == '-') return NAN;
  return std::atof(p);
}

std::string lastLine() {
  size_t end = sim::serialOut.rfind("\r\n");
  if (end == std::string::npos) return "";
  size_t start = sim::serialOut.rfind("\r\n", end - 1);
  return sim::serialOut.substr(start == std::string::npos ? 0 : start + 2, end - (start == std::string::npos ? 0 : start + 2));
}

#define CHECK_NEAR(a, b, tol)                                                                  \
  do {                                                                                         \
    double _a = (a), _b = (b);                                                                 \
    if (!(std::fabs(_a - _b) <= (tol)))                                                        \
      hwtest::fail(__FILE__, __LINE__, "CHECK_NEAR got " + std::to_string(_a) + " vs " +      \
                                           std::to_string(_b));                                \
  } while (0)

void runAt(double c, unsigned long ms = 3000) {
  truck.rs = ohmsFor(c);
  sim::runFor(ms);
}

}  // namespace

// ---------------- conversion ----------------

TEST(boot_message) {
  attach();
  setup();
  CHECK_CONTAINS(sim::serialOut, "hj60_coolant boot ok");
}

TEST(fsm_points_convert_exactly) {
  CHECK_NEAR(ohmsToC(226.0), 50.0, 0.01);
  CHECK_NEAR(ohmsToC(26.0), 115.0, 0.01);
}

TEST(owner_measurement_150_ohm_is_about_60C) {
  CHECK_NEAR(ohmsToC(150.0), 60.6, 0.2);
}

TEST(steady_feed_reads_temperature) {
  attach();
  setup();
  runAt(90.0);
  CHECK_NEAR(lastCoolant(), 90.0, 0.5);
  CHECK_CONTAINS(lastLine(), "State:OK");
  CHECK_EQ(sim::pinLevel[BUZZER], LOW);
}

TEST(pulsing_regulator_reads_same_as_steady) {
  // Bimetal regulator: on 400 ms, off 1600 ms. Off-phases are thrown away
  // and must not raise a NOFEED fault.
  truck.onMs = 400; truck.periodMs = 2000;
  attach();
  setup();
  runAt(90.0, 8000);
  CHECK_NEAR(lastCoolant(), 90.0, 0.5);
  CHECK_NOT_CONTAINS(sim::serialOut, "FAULT");
}

TEST(feed_level_does_not_change_reading) {
  attach();
  setup();
  truck.feedOn = 7.0;
  runAt(95.0);
  double low = lastCoolant();
  truck.feedOn = 14.4;   // regulator contacts closed, engine running (12 V system)
  sim::runFor(3000);
  CHECK_NEAR(low, 95.0, 0.6);
  CHECK_NEAR(lastCoolant(), 95.0, 0.6);
}

TEST(contact_bounce_rejected_by_median) {
  truck.bounceEvery = 4;   // 25% of sender reads glitch to 0 V
  attach();
  setup();
  runAt(90.0);
  CHECK_NEAR(lastCoolant(), 90.0, 0.5);
  CHECK_NOT_CONTAINS(sim::serialOut, "FAULT-SHORT");
}

TEST(feed_edge_inside_pair_is_rejected) {
  // Feed collapses between the two feed reads of every pair: no pair is
  // valid, so no (wrong) temperature is ever reported.
  int calls = 0;
  sim::analogMvFn = [&calls](uint8_t pin) {
    if (pin == FEED) return (calls++ % 2 == 0) ? mv(11.7) : mv(5.0);
    return mv(11.7 * 0.686);
  };
  setup();
  sim::runFor(3000);
  CHECK(std::isnan(lastCoolant()));
  CHECK_CONTAINS(lastLine(), "State:WAIT");
}

// ---------------- thresholds ----------------

TEST(just_below_warn_is_ok) {
  attach();
  setup();
  runAt(99.0);
  CHECK_CONTAINS(lastLine(), "State:OK");
  CHECK_EQ(sim::togglesBetween(BUZZER, 0, sim::now), 0);
}

TEST(warn_beeps) {
  attach();
  setup();
  runAt(101.0);
  CHECK_CONTAINS(lastLine(), "State:WARN");
  unsigned long t = sim::now;
  sim::runFor(2000);
  int toggles = sim::togglesBetween(BUZZER, t, sim::now);
  CHECK(toggles >= 4 && toggles <= 6);
}

TEST(alarm_is_steady) {
  attach();
  setup();
  runAt(106.0);
  CHECK_CONTAINS(lastLine(), "State:ALARM");
  unsigned long t = sim::now;
  sim::runFor(2000);
  CHECK_EQ(sim::pinLevel[BUZZER], HIGH);
  CHECK_EQ(sim::togglesBetween(BUZZER, t, sim::now), 0);
}

TEST(alarm_hysteresis) {
  attach();
  setup();
  runAt(106.0);
  runAt(104.0);   // inside the 2 C band: stays ALARM
  CHECK_CONTAINS(lastLine(), "State:ALARM");
  runAt(102.0);   // below 103: drops to WARN
  CHECK_CONTAINS(lastLine(), "State:WARN");
  runAt(98.5);    // inside the WARN band: stays WARN
  CHECK_CONTAINS(lastLine(), "State:WARN");
  runAt(97.0);
  CHECK_CONTAINS(lastLine(), "State:OK");
  sim::runFor(500);
  CHECK_EQ(sim::pinLevel[BUZZER], LOW);
}

// ---------------- faults: fail loud ----------------

TEST(no_feed_is_a_fault_after_5s) {
  attach();
  truck.feedOn = 0.0;
  setup();
  sim::runFor(4500);
  CHECK_NOT_CONTAINS(sim::serialOut, "FAULT");
  sim::runFor(2000);
  CHECK_CONTAINS(lastLine(), "Coolant: --");
  CHECK_CONTAINS(lastLine(), "State:FAULT-NOFEED");
  unsigned long t = sim::now;
  sim::runFor(5000);
  CHECK_EQ(sim::togglesBetween(BUZZER, t, sim::now), 4);  // double chirp
}

TEST(feed_lost_while_running_is_a_fault) {
  attach();
  setup();
  runAt(90.0);
  truck.feedOn = 0.0;
  sim::runFor(7000);
  CHECK_CONTAINS(lastLine(), "State:FAULT-NOFEED");
}

TEST(open_sender_is_a_fault) {
  attach();
  setup();
  truck.rs = INFINITY;
  sim::runFor(3000);
  CHECK_CONTAINS(lastLine(), "State:FAULT-OPEN");
  CHECK_CONTAINS(lastLine(), "Coolant: --");
}

TEST(shorted_sender_is_a_fault) {
  attach();
  setup();
  truck.rs = 0.0;
  sim::runFor(3000);
  CHECK_CONTAINS(lastLine(), "State:FAULT-SHORT");
}

TEST(recovers_from_fault) {
  attach();
  setup();
  truck.rs = INFINITY;
  sim::runFor(3000);
  runAt(90.0);
  CHECK_CONTAINS(lastLine(), "State:OK");
  sim::runFor(500);
  CHECK_EQ(sim::pinLevel[BUZZER], LOW);
}

// ---------------- display ----------------

TEST(display_shows_temperature_and_state) {
  attach();
  sim::i2cDevices = {0x3C};
  setup();
  runAt(92.0);
  CHECK_CONTAINS(sim::lastFrameText, "92C");
  CHECK_CONTAINS(sim::lastFrameText, "OK");
}

TEST(missing_display_still_alarms) {
  attach();
  sim::displayBeginOk = false;
  setup();
  CHECK_CONTAINS(sim::serialOut, "OLED not found");
  runAt(106.0);
  CHECK_CONTAINS(lastLine(), "State:ALARM");
  CHECK_EQ(sim::pinLevel[BUZZER], HIGH);
}

TEST(log_format) {
  attach();
  setup();
  runAt(90.0);
  std::string l = lastLine();
  CHECK_CONTAINS(l, "Coolant: ");
  CHECK_CONTAINS(l, "C  Rs:");
  CHECK_CONTAINS(l, "ohm  k:0.");
  CHECK_CONTAINS(l, "  Feed:11.7V  State:OK");
}
