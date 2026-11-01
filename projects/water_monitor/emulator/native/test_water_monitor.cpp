// Native tests for water_monitor. Compiled against the real .ino with
// the mocks in tools/native/mocks. Run: make test P=water_monitor
//
// Timeline reminder: setup() spends 1800 ms in delay(), loop() ~40 ms.
// Water is read every >500 ms, air + serial log every >2000 ms.
#include "hwtest.h"

static constexpr uint8_t BUZZER = 3;

static void boot(float waterC, float airC = 22.0f, float hum = 48.0f) {
  sim::ds18b20C[0] = waterC;
  sim::dhtTempC = airC;
  sim::dhtHumidity = hum;
  setup();
}

// Run loop() for `ms`, return true if any pushed frame contained `text`.
static bool screenShowedDuring(unsigned long ms, const char *text) {
  bool seen = false;
  unsigned long end = sim::now + ms;
  while (sim::now < end) {
    loop();
    if (sim::lastFrameText.find(text) != std::string::npos) seen = true;
  }
  return seen;
}

static std::string lastLogLine() {
  const std::string &s = sim::serialOut;
  size_t end = s.rfind("\r\n");
  if (end == std::string::npos) return "";
  size_t start = s.rfind("\r\n", end - 1);
  start = (start == std::string::npos) ? 0 : start + 2;
  return s.substr(start, end - start);
}

TEST(boot_splash_shown) {
  boot(25);
  CHECK_CONTAINS(sim::lastFrameText, "LC78 OFFROAD");
}

TEST(cool_water_buzzer_silent_and_log_format) {
  boot(25);
  sim::runFor(5000);
  CHECK_EQ(sim::togglesBetween(BUZZER, 0, sim::now), 0);
  CHECK_EQ(sim::pinLevel[BUZZER], LOW);
  CHECK_CONTAINS(sim::serialOut,
                 "Water: 25.0C/77.0F   Air: 22.0C/71.6F  Hum:48%   Buzzer: OFF");
}

TEST(warm_water_beeps_about_every_400ms) {
  boot(37);
  sim::runFor(1000);
  unsigned long t0 = sim::now;
  sim::runFor(4000);
  int toggles = sim::togglesBetween(BUZZER, t0, sim::now);
  // 400 ms threshold + 40 ms loop granularity -> ~440 ms per toggle.
  CHECK(toggles >= 8 && toggles <= 11);
  CHECK_CONTAINS(lastLogLine(), "Buzzer: BEEP");
}

TEST(warm_water_flashes_WARM_on_screen) {
  boot(37);
  sim::runFor(600);
  CHECK(screenShowedDuring(2000, "WARM"));
}

TEST(hot_water_buzzer_continuous) {
  boot(45);
  sim::runFor(1000);
  unsigned long t0 = sim::now;
  sim::runFor(3000);
  CHECK_EQ(sim::pinLevel[BUZZER], HIGH);
  CHECK_EQ(sim::togglesBetween(BUZZER, t0, sim::now), 0);
  CHECK_CONTAINS(lastLogLine(), "Buzzer: LOUD-CONT");
  CHECK(screenShowedDuring(2000, "W HOT!"));
}

TEST(threshold_35_0_is_beep) {
  boot(35.0f);
  sim::runFor(3000);
  CHECK_CONTAINS(lastLogLine(), "Buzzer: BEEP");
}

TEST(threshold_34_9_is_off) {
  boot(34.9f);
  sim::runFor(3000);
  CHECK_CONTAINS(lastLogLine(), "Buzzer: OFF");
}

TEST(threshold_40_0_is_still_beep) {
  boot(40.0f);
  sim::runFor(3000);
  CHECK_CONTAINS(lastLogLine(), "Buzzer: BEEP");
}

TEST(threshold_40_1_is_alarm) {
  boot(40.1f);
  sim::runFor(3000);
  CHECK_CONTAINS(lastLogLine(), "Buzzer: LOUD-CONT");
}

TEST(cooling_down_silences_buzzer) {
  boot(45);
  sim::runFor(3000);
  CHECK_EQ(sim::pinLevel[BUZZER], HIGH);
  sim::ds18b20C[0] = 25;
  sim::runFor(3000);
  CHECK_EQ(sim::pinLevel[BUZZER], LOW);
  CHECK_CONTAINS(lastLogLine(), "Buzzer: OFF");
}

TEST(probe_unplugged_shows_dashes_and_stays_silent) {
  boot(NAN);
  sim::runFor(3000);
  CHECK_CONTAINS(lastLogLine(), "Water: --");
  CHECK_CONTAINS(lastLogLine(), "Buzzer: OFF");
  CHECK_CONTAINS(sim::lastFrameText, "W:--");
  CHECK_EQ(sim::togglesBetween(BUZZER, 0, sim::now), 0);
}

TEST(dht_failure_shows_dashes) {
  boot(25, NAN, NAN);
  sim::runFor(3000);
  CHECK_CONTAINS(lastLogLine(), "Air: --");
  CHECK_CONTAINS(sim::lastFrameText, "A:--");
  CHECK_CONTAINS(sim::lastFrameText, "H:--");
}

TEST(serial_log_roughly_every_2s) {
  boot(25);
  sim::runFor(10000);
  int lines = 0;
  for (size_t p = 0; (p = sim::serialOut.find("Water: ", p)) != std::string::npos; p++) lines++;
  CHECK(lines >= 4 && lines <= 5);
}
