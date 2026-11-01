// =====================================================================
//  SIM-ONLY SHIM -- never copy into firmware/.
// ---------------------------------------------------------------------
//  Wokwi has no SH1106 part, only SSD1306 (same 128x64, I2C @ 0x3C).
//  Passed to arduino-cli via --library (highest priority), it shadows the real
//  Adafruit_SH110X library, so the sim .ino stays byte-identical to the
//  firmware .ino. See docs/adr/0005-sim-display-shim.md.
// =====================================================================
#pragma once
#include <Adafruit_SSD1306.h>

#define SH110X_WHITE   SSD1306_WHITE
#define SH110X_BLACK   SSD1306_BLACK
#define SH110X_INVERSE SSD1306_INVERSE

class Adafruit_SH1106G : public Adafruit_SSD1306 {
public:
  Adafruit_SH1106G(uint16_t w, uint16_t h, TwoWire *twi = &Wire, int8_t rst_pin = -1)
      : Adafruit_SSD1306(w, h, twi, rst_pin) {}

  // SH110X signature: begin(addr, reset). SSD1306 wants (vcc, addr, reset).
  bool begin(uint8_t addr = 0x3C, bool reset = true) {
    return Adafruit_SSD1306::begin(SSD1306_SWITCHCAPVCC, addr, reset);
  }
};
