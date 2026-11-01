// SIM-ONLY Wokwi custom chip: the mode B measurement circuit of hj60_coolant.
//
// Real circuit: GPIO7 drives Rref 220 ohm -> sender (NTC, FSM 226 ohm @
// 50 C, 26 ohm @ 115 C) -> ground. The ESP32 reads Vtop (GPIO4) and Vs
// (GPIO5), each through a 47k/100k divider (1.47:1).
//
// Wokwi cannot simulate that resistor network, and its potentiometers ignore
// their VCC pin, so this chip computes the two voltages:
//   EN high: VTOP = 3.2 V / 1.47, VS = 3.2 V * Rs / (Rs + 220) / 1.47
//   EN low : VTOP = 0, VS = 0
//   "open"     : Rs = infinite (VS = 3.2 V while EN is high)
//   "backfeed" : the wire is at least this voltage, EN high or low
// See docs/adr/0011-hj60-wokwi-sender-chip.md.
#include "wokwi-api.h"
#include <math.h>
#include <stdlib.h>

#define VTOP 3.2f
#define RREF 220.0f
#define DIV 1.47f

typedef struct {
  pin_t en, vtop, vs;
  uint32_t temperature, open, backfeed;
} chip_t;

static float sender_ohms(float c) {
  const float t0 = 323.15f, t1 = 388.15f;
  const float b = logf(226.0f / 26.0f) / (1.0f / t0 - 1.0f / t1);
  return 226.0f * expf(b * (1.0f / (c + 273.15f) - 1.0f / t0));
}

static void update(void *user_data) {
  chip_t *chip = (chip_t *)user_data;
  int on = pin_read(chip->en);
  float vt = on ? VTOP : 0.0f;
  float vs;
  if (attr_read_float(chip->open) > 0.5f) vs = vt;
  else {
    float rs = sender_ohms(attr_read_float(chip->temperature));
    vs = vt * rs / (rs + RREF);
  }
  float back = attr_read_float(chip->backfeed);
  if (back > vs) vs = back;
  pin_dac_write(chip->vtop, vt / DIV);
  pin_dac_write(chip->vs, vs / DIV);
}

static void on_en(void *user_data, pin_t pin, uint32_t value) { update(user_data); }

void chip_init(void) {
  chip_t *chip = malloc(sizeof(chip_t));
  chip->en = pin_init("EN", INPUT_PULLDOWN);
  chip->vtop = pin_init("VTOP", ANALOG);
  chip->vs = pin_init("VS", ANALOG);
  chip->temperature = attr_init_float("temperature", 90.0f);
  chip->open = attr_init_float("open", 0.0f);
  chip->backfeed = attr_init_float("backfeed", 0.0f);

  const pin_watch_config_t watch = { .edge = BOTH, .pin_change = on_en, .user_data = chip };
  pin_watch(chip->en, &watch);

  // Controls can change at any time: refresh every 10 ms as well.
  const timer_config_t cfg = { .callback = update, .user_data = chip };
  timer_t t = timer_init(&cfg);
  timer_start(t, 10000, true);
  update(chip);
}
