#pragma once
#include "Adafruit_GFX.h"
#include "Wire.h"

#define SH110X_BLACK 0
#define SH110X_WHITE 1
#define SH110X_INVERSE 2

class Adafruit_SH110X : public Adafruit_GFX {
public:
  Adafruit_SH110X(uint16_t w, uint16_t h, TwoWire * = &Wire, int8_t = -1) : Adafruit_GFX(w, h) {}
  bool begin(uint8_t = 0x3C, bool = true) { return sim::displayBeginOk; }
  void clearDisplay() { sim::frameText.clear(); }
  void display() { sim::lastFrameText = sim::frameText; sim::framesShown++; }
  void setContrast(uint8_t) {}
  void invertDisplay(bool) {}
};

class Adafruit_SH1106G : public Adafruit_SH110X { using Adafruit_SH110X::Adafruit_SH110X; };
class Adafruit_SH1107 : public Adafruit_SH110X { using Adafruit_SH110X::Adafruit_SH110X; };
