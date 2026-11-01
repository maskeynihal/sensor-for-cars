#pragma once
#include "Arduino.h"

class OneWire {
public:
  explicit OneWire(uint8_t pin) : pin_(pin) {}
  uint8_t pin() const { return pin_; }

private:
  uint8_t pin_;
};
