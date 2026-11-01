// =====================================================================
//  SIM-ONLY SHIM -- never copy into firmware/.
// ---------------------------------------------------------------------
//  Wokwi has no ILI9488 part, only an ILI9341 (240x320). Passed to
//  arduino-cli via --library (highest priority), this header shadows the
//  real <DIYables_TFT_SPI.h> umbrella. It pulls in the real ILI9341 driver
//  and renames DIYables_ILI9488_SPI to a subclass of it, so the sim .ino
//  stays byte-identical to the firmware .ino. The subclass has its own
//  name, so it never collides with the real ILI9488 driver at link time.
//  See docs/adr/0010-hj60-ili9488-tft-and-sim-shim.md.
// =====================================================================
#pragma once
#include <DIYables_ILI9341_SPI.h>

class SimShim_ILI9488 : public DIYables_ILI9341_SPI {
public:
  // The sketch passes the ILI9488 native size (320x480). Wokwi's panel is 240x320.
  SimShim_ILI9488(uint16_t, uint16_t, int8_t cs, int8_t dc, int8_t rst = -1, SPIClass *bus = &SPI)
      : DIYables_ILI9341_SPI(240, 320, cs, dc, rst, bus) {}
};

#define DIYables_ILI9488_SPI SimShim_ILI9488
