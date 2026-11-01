#pragma once
#include "Adafruit_GFX.h"
#include "Wire.h"

#define SSD1306_BLACK 0
#define SSD1306_WHITE 1
#define SSD1306_INVERSE 2
#define SSD1306_SWITCHCAPVCC 0x02
#define SSD1306_EXTERNALVCC 0x01

class Adafruit_SSD1306 : public Adafruit_GFX {
public:
  Adafruit_SSD1306(uint8_t w, uint8_t h, TwoWire * = &Wire, int8_t = -1) : Adafruit_GFX(w, h) {}
  bool begin(uint8_t = SSD1306_SWITCHCAPVCC, uint8_t = 0, bool = true, bool = true) { return sim::displayBeginOk; }
  void clearDisplay() { sim::frameText.clear(); }
  void display() { sim::lastFrameText = sim::frameText; sim::framesShown++; }
  void invertDisplay(bool) {}
  void dim(bool) {}
};
