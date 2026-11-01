// Minimal host-side Arduino core for native tests. Covers what the
// sketches in this repo use; extend when a sketch needs more.
#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "sim.h"

typedef bool boolean;
typedef uint8_t byte;
typedef unsigned int word;

#define HIGH 0x1
#define LOW 0x0
#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

#define LED_BUILTIN 13
#define A0 14
#define A1 15
#define A2 16
#define A3 17
#define A4 18
#define A5 19

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

#define PROGMEM
#define F(s) (s)
#define PI 3.1415926535897932384626433832795

using std::abs;
template <class T, class U> auto min(T a, U b) -> decltype(a < b ? a : b) { return a < b ? a : b; }
template <class T, class U> auto max(T a, U b) -> decltype(a > b ? a : b) { return a > b ? a : b; }
template <class T, class L, class H> T constrain(T x, L lo, H hi) { return x < lo ? lo : (x > hi ? hi : x); }
long map(long x, long in_min, long in_max, long out_min, long out_max);

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
int analogRead(uint8_t pin);
void analogWrite(uint8_t pin, int val);

unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);

long random(long max);
long random(long min, long max);
void randomSeed(unsigned long seed);

class Print {
public:
  virtual ~Print() = default;
  virtual size_t write(uint8_t c) = 0;
  size_t write(const char *s) { size_t n = 0; while (*s) n += write((uint8_t)*s++); return n; }

  size_t print(const char *s) { return write(s); }
  size_t print(const std::string &s) { return write(s.c_str()); }
  size_t print(char c) { return write((uint8_t)c); }
  size_t print(unsigned char v, int base = DEC) { return print((unsigned long)v, base); }
  size_t print(int v, int base = DEC) { return print((long)v, base); }
  size_t print(unsigned int v, int base = DEC) { return print((unsigned long)v, base); }
  size_t print(long v, int base = DEC);
  size_t print(unsigned long v, int base = DEC);
  size_t print(double v, int digits = 2);

  size_t println() { return write("\r\n"); }
  template <class T> size_t println(T v) { size_t n = print(v); return n + println(); }
  template <class T> size_t println(T v, int fmt) { size_t n = print(v, fmt); return n + println(); }
};

class HardwareSerial : public Print {
public:
  void begin(unsigned long) {}
  void end() {}
  int available() { return (int)sim::serialIn.size(); }
  int read();
  int peek() { return sim::serialIn.empty() ? -1 : (uint8_t)sim::serialIn[0]; }
  void flush() {}
  size_t write(uint8_t c) override { sim::serialOut.push_back((char)c); return 1; }
  using Print::write;
  explicit operator bool() const { return true; }
};

extern HardwareSerial Serial;

// Sketch entry points (defined by the .ino under test).
void setup();
void loop();
