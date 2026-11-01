// Native tests for the hj60_coolant TFT dashboard. The TFT mock keeps the
// text at each cursor position, so tftmock::screen() is what the panel shows.
#include "truck_model.h"

namespace {
std::string scr() { return tftmock::screen(); }
}

TEST(ui_boot_then_check_then_main) {
  attachB();
  direct.rs = ohmsFor(90.0);
  setup();
  sim::runFor(200);
  CHECK_NOT_CONTAINS(scr(), "CHECKING");          // boot art only
  sim::runFor(1500);
  CHECK_CONTAINS(scr(), "CHECKING SYSTEMS...");
  CHECK_CONTAINS(scr(), "MODE B: DASH GAUGE DISCONNECTED");
  sim::runFor(3000);
  CHECK_CONTAINS(scr(), "SENDER OK  BATT 12.9V");
  sim::runFor(1500);
  std::string s = scr();
  CHECK_CONTAINS(s, "RPM");
  CHECK_CONTAINS(s, "SPD");
  CHECK_CONTAINS(s, "WATER");
  CHECK_CONTAINS(s, "BATT");
  CHECK_CONTAINS(s, "DEMO");
  CHECK_CONTAINS(s, "\n90\n");
  CHECK_CONTAINS(s, "12.9");
}

TEST(ui_mode_a_check_screen) {
  attach();
  setup();
  sim::runFor(1800);
  CHECK_CONTAINS(scr(), "MODE A: PASSIVE TAPS, DASH GAUGE ON");
}

TEST(ui_button_cycles_screens) {
  attachB();
  direct.rs = ohmsFor(90.0);
  setup();
  sim::runFor(6500);
  CHECK_CONTAINS(scr(), "SPD");
  pressButton();
  CHECK_CONTAINS(scr(), "BATTERY");           // GRID
  CHECK_CONTAINS(scr(), "WATER TEMP");
  pressButton();
  CHECK_NOT_CONTAINS(scr(), "BATTERY");       // WATER
  CHECK_CONTAINS(scr(), "SPEED");
  CHECK_CONTAINS(scr(), "WATER TEMP");
  pressButton();
  CHECK_CONTAINS(scr(), "SPD");               // back to MAIN
}

TEST(ui_button_ignored_during_boot) {
  attachB();
  setup();
  sim::runFor(300);
  pressButton();
  sim::runFor(6000);
  CHECK_CONTAINS(scr(), "SPD");               // still MAIN, not GRID
}

TEST(ui_alarm_switches_to_water_screen) {
  attachB();
  direct.rs = ohmsFor(90.0);
  setup();
  sim::runFor(6500);
  CHECK_CONTAINS(scr(), "SPD");
  runAt(107.0, 2500);
  CHECK_CONTAINS(scr(), "WATER HOT!");
  CHECK_CONTAINS(scr(), "\n107\n");
  CHECK_NOT_CONTAINS(scr(), "SPD");
  runAt(90.0, 2500);
  CHECK_CONTAINS(scr(), "SPD");               // back to the screen the driver chose
}

TEST(ui_fault_label_on_screen) {
  attachB();
  direct.rs = ohmsFor(90.0);
  setup();
  sim::runFor(6500);
  direct.rs = INFINITY;
  sim::runFor(2500);
  CHECK_CONTAINS(scr(), "SENDER OPEN");
  CHECK_CONTAINS(scr(), "\n--\n");
}

TEST(ui_backlight_on) {
  attachB();
  setup();
  CHECK_EQ(tftmock::backlight, 255);
}

TEST(ui_values_redraw_only_on_change) {
  attachB();
  direct.rs = ohmsFor(90.0);
  setup();
  sim::runFor(6500);
  unsigned long clears = tftmock::fullClears;
  sim::runFor(3000);
  CHECK_EQ(tftmock::fullClears, clears);      // no full-screen redraw while nothing changes screens
}

TEST(ui_usb_power_without_battery_divider) {
  attachB();
  batterySense = false;
  direct.rs = ohmsFor(90.0);
  setup();
  sim::runFor(4700);
  CHECK_CONTAINS(scr(), "SENDER OK  USB POWER");
  sim::runFor(1800);
  CHECK_CONTAINS(scr(), "\nUSB\n");
  CHECK_CONTAINS(lastLine(), "Batt:--");
}

