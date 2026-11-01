// Headless Adafruit_GFX: drawing is a no-op, text is captured into
// sim::frameText so tests can assert what the screen says.
#pragma once
#include "Arduino.h"

class Adafruit_GFX : public Print {
public:
  Adafruit_GFX(int16_t w, int16_t h) : w_(w), h_(h) {}

  int16_t width() const { return w_; }
  int16_t height() const { return h_; }

  void drawPixel(int16_t, int16_t, uint16_t) {}
  void drawLine(int16_t, int16_t, int16_t, int16_t, uint16_t) {}
  void drawFastHLine(int16_t, int16_t, int16_t, uint16_t) {}
  void drawFastVLine(int16_t, int16_t, int16_t, uint16_t) {}
  void drawRect(int16_t, int16_t, int16_t, int16_t, uint16_t) {}
  void fillRect(int16_t, int16_t, int16_t, int16_t, uint16_t) {}
  void drawCircle(int16_t, int16_t, int16_t, uint16_t) {}
  void fillCircle(int16_t, int16_t, int16_t, uint16_t) {}
  void drawRoundRect(int16_t, int16_t, int16_t, int16_t, int16_t, uint16_t) {}
  void fillRoundRect(int16_t, int16_t, int16_t, int16_t, int16_t, uint16_t) {}
  void drawTriangle(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t, uint16_t) {}
  void fillTriangle(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t, uint16_t) {}
  void drawBitmap(int16_t, int16_t, const uint8_t *, int16_t, int16_t, uint16_t) {}
  void fillScreen(uint16_t) { sim::frameText.clear(); }

  void setCursor(int16_t x, int16_t y) { cx_ = x; cy_ = y; sim::frameText.push_back('\n'); }
  void setTextSize(uint8_t) {}
  void setTextColor(uint16_t) {}
  void setTextColor(uint16_t, uint16_t) {}
  void setTextWrap(bool) {}
  void setRotation(uint8_t) {}
  int16_t getCursorX() const { return cx_; }
  int16_t getCursorY() const { return cy_; }

  size_t write(uint8_t c) override { sim::frameText.push_back((char)c); return 1; }
  using Print::write;

protected:
  int16_t w_, h_;
  int16_t cx_ = 0, cy_ = 0;
};
