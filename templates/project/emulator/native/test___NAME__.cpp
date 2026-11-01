// Native tests for __NAME__. Compiled against the real .ino with the
// mocks in tools/native/mocks. Run: make test P=__NAME__
#include "hwtest.h"

TEST(boot_message) {
  setup();
  CHECK_CONTAINS(sim::serialOut, "__NAME__ boot ok");
}

TEST(heartbeat_toggles_status_led_every_second) {
  setup();
  sim::runFor(5500);
  CHECK_EQ(sim::togglesBetween(LED_BUILTIN, 0, sim::now), 5);
  CHECK_CONTAINS(sim::serialOut, "heartbeat 5");
}
