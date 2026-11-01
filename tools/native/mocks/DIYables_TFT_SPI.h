// Headless DIYables_TFT_SPI (ILI9341 / ILI9488 / ST7789). Drawing is a no-op.
// Text is kept per cursor position, like pixels on a real TFT: printing at a
// position replaces what was there, and fillRect/fillScreen erase the text
// anchored inside them. tftmock::screen() returns what the panel shows now.
#pragma once
#include <map>
#include <utility>

#include "Adafruit_GFX.h"
#include "SPI.h"

namespace tftmock {
inline std::map<std::pair<int, int>, std::string> text;  // key = (y, x) of the cursor
inline std::pair<int, int> cursor{0, 0};
inline int backlight = -1;
inline unsigned long fullClears = 0;

// Every text item, top-to-bottom then left-to-right, one per line.
inline std::string screen() {
  std::string out;
  for (auto &kv : text) { out += kv.second; out += '\n'; }
  return out;
}
}  // namespace tftmock

class DIYables_TFT_SPI : public Adafruit_GFX {
public:
  DIYables_TFT_SPI(uint16_t w, uint16_t h, int8_t, int8_t, int8_t = -1, SPIClass * = &SPI)
      : Adafruit_GFX(w, h), nw_(w), nh_(h) {}
  void begin(uint32_t = 0) {}
  void setRotation(uint8_t r) {
    bool swap = r & 1;
    w_ = swap ? nh_ : nw_;
    h_ = swap ? nw_ : nh_;
  }
  void fillScreen(uint16_t) { tftmock::text.clear(); tftmock::fullClears++; }
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t) {
    for (auto it = tftmock::text.begin(); it != tftmock::text.end();) {
      int ty = it->first.first, tx = it->first.second;
      if (tx >= x && tx < x + w && ty >= y && ty < y + h) it = tftmock::text.erase(it);
      else ++it;
    }
  }
  void setCursor(int16_t x, int16_t y) {
    cx_ = x; cy_ = y;
    tftmock::cursor = {y, x};
    tftmock::text[tftmock::cursor].clear();
  }
  size_t write(uint8_t c) override {
    if (c != '\r' && c != '\n') tftmock::text[tftmock::cursor].push_back((char)c);
    return 1;
  }
  using Print::write;
  void initBacklight(int8_t) {}
  void setBacklight(uint8_t b) { tftmock::backlight = b; }
  void backlightOn() { tftmock::backlight = 255; }
  void backlightOff() { tftmock::backlight = 0; }
  static uint16_t colorRGB(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }

private:
  uint16_t nw_, nh_;
};

class DIYables_ILI9488_SPI : public DIYables_TFT_SPI { using DIYables_TFT_SPI::DIYables_TFT_SPI; };
class DIYables_ILI9341_SPI : public DIYables_TFT_SPI { using DIYables_TFT_SPI::DIYables_TFT_SPI; };
class DIYables_ST7789_SPI : public DIYables_TFT_SPI { using DIYables_TFT_SPI::DIYables_TFT_SPI; };
