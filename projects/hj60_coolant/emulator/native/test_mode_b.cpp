// Native tests for hj60_coolant, sender mode B (direct: the YG wire goes to
// the ESP32, the dash gauge is disconnected).
#include "truck_model.h"

TEST(b_boot_message_names_mode) {
  attachB();
  setup();
  CHECK_CONTAINS(sim::serialOut, "Mode:B (direct, DASH GAUGE DISCONNECTED)");
}

TEST(b_reads_temperature) {
  attachB();
  setup();
  runAt(90.0);
  CHECK_NEAR(lastCoolant(), 90.0, 0.5);
  CHECK_CONTAINS(lastLine(), "Vtop:3.20V");
  CHECK_CONTAINS(lastLine(), "Mode:B  State:OK");
  CHECK_EQ(sim::pinLevel[BUZZER], LOW);
}

TEST(b_owner_value_150_ohm) {
  attachB();
  setup();
  direct.rs = 150.0;
  sim::runFor(3000);
  CHECK_NEAR(lastCoolant(), 60.6, 0.5);
}

TEST(b_excitation_level_does_not_change_reading) {
  attachB();
  setup();
  direct.vtop = 2.7;
  runAt(100.0);
  double low = lastCoolant();
  direct.vtop = 3.2;
  sim::runFor(3000);
  CHECK_NEAR(low, 100.0, 0.6);
  CHECK_NEAR(lastCoolant(), 100.0, 0.6);
}

TEST(b_excitation_is_short_pulses) {
  attachB();
  setup();
  runAt(90.0, 2000);
  unsigned long t = sim::now;
  sim::runFor(1000);
  // one pulse (on + off) every 100 ms
  int toggles = sim::togglesBetween(EXCITE, t, sim::now);
  CHECK(toggles >= 18 && toggles <= 22);
  // total on time is a small part of the second
  unsigned long onMs = 0, onAt = 0;
  bool on = false;
  for (auto &e : sim::pinLog) {
    if (e.pin != EXCITE || e.t < t) continue;
    if (e.value == HIGH) { on = true; onAt = e.t; }
    else if (on) { on = false; onMs += e.t - onAt; }
  }
  CHECK(onMs <= 100);
}

TEST(b_warn_and_alarm) {
  attachB();
  setup();
  runAt(101.0);
  CHECK_CONTAINS(lastLine(), "State:WARN");
  runAt(106.0);
  CHECK_CONTAINS(lastLine(), "State:ALARM");
  sim::runFor(500);
  CHECK_EQ(sim::pinLevel[BUZZER], HIGH);
}

TEST(b_open_sender_is_a_fault) {
  attachB();
  setup();
  direct.rs = INFINITY;
  sim::runFor(3000);
  CHECK_CONTAINS(lastLine(), "State:FAULT-OPEN");
  CHECK_CONTAINS(lastLine(), "Coolant: --");
}

TEST(b_shorted_sender_is_a_fault) {
  attachB();
  setup();
  direct.rs = 0.0;
  sim::runFor(3000);
  CHECK_CONTAINS(lastLine(), "State:FAULT-SHORT");
}

TEST(b_no_excitation_is_a_fault_after_5s) {
  attachB();
  direct.exciteDead = true;
  setup();
  sim::runFor(4500);
  CHECK_NOT_CONTAINS(sim::serialOut, "FAULT");
  sim::runFor(2000);
  CHECK_CONTAINS(lastLine(), "State:FAULT-NOEXCITE");
}

TEST(b_wire_shorted_to_battery_is_backfeed) {
  attachB();
  setup();
  runAt(90.0);
  direct.backfeedV = 12.9;   // clamped to 3.3 V at the pin
  sim::runFor(3000);
  CHECK_CONTAINS(lastLine(), "State:FAULT-BACKFEED");
  CHECK_CONTAINS(lastLine(), "Coolant: --");
}

TEST(b_gauge_still_connected_is_backfeed) {
  // Regulator pulses (60% on) reach the sender wire when F is still connected.
  attachB();
  direct.backfeedV = 9.0;
  direct.backOnMs = 840;
  direct.backPeriodMs = 1400;
  setup();
  runAt(90.0, 4000);
  CHECK_CONTAINS(lastLine(), "State:FAULT-BACKFEED");
}

TEST(b_recovers_from_backfeed) {
  attachB();
  setup();
  direct.backfeedV = 12.9;
  runAt(90.0);
  direct.backfeedV = 0;
  sim::runFor(3000);
  CHECK_CONTAINS(lastLine(), "State:OK");
  sim::runFor(500);
  CHECK_EQ(sim::pinLevel[BUZZER], LOW);
}

TEST(b_battery_voltage_logged) {
  attachB();
  direct.batt = 13.8;
  setup();
  runAt(90.0);
  CHECK_CONTAINS(lastLine(), "Batt:13.8V");
}

TEST(b_cold_engine_corrects_divider_load) {
  // At 20 C the sender is ~850 ohm, so the 17.6k Vs divider in parallel
  // matters (~1 C). The firmware removes it.
  attachB();
  setup();
  runAt(20.0);
  CHECK_NEAR(lastCoolant(), 20.0, 0.5);
  CHECK_CONTAINS(lastLine(), "State:OK");
}
