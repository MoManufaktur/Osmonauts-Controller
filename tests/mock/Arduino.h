// Osmonauts – host-side mock of the Arduino API for simulation tests
// (c) 2026 Kai Mommsen & Elias Dörre · SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

typedef uint8_t byte;
typedef bool boolean;
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define A0 14
#define A1 15
#define A2 16
#define A3 17
#define A4 18
#define A5 19
#define A6 20
#define A7 21

class __FlashStringHelper;
#define F(s) (reinterpret_cast<const __FlashStringHelper *>(s))

extern unsigned long sim_ms;
extern int sim_pin_out[64];
extern int sim_pin_in[64];
extern int sim_analog[64];
extern std::string sim_serial;
void sim_on_write(uint8_t pin);

inline unsigned long millis() { return sim_ms; }
inline void delay(unsigned long d) { sim_ms += d; }
inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t p, uint8_t v) { if (sim_pin_out[p] != v) { sim_pin_out[p] = v; sim_on_write(p); } }
inline int digitalRead(uint8_t p) { return sim_pin_in[p]; }
inline int analogRead(uint8_t p) { return sim_analog[p]; }
inline void tone(uint8_t, unsigned int) {}
inline void noTone(uint8_t) {}
inline long random(long a, long) { return a; }

struct SerialMock {
  void begin(long) {}
  void add(const std::string &s) { if (sim_serial.size() < 4000000) sim_serial += s; }
  void print(const __FlashStringHelper *s) { add((const char *)s); }
  void print(const char *s) { add(s); }
  void print(long v) { add(std::to_string(v)); }
  void print(int v) { add(std::to_string(v)); }
  void print(unsigned long v) { add(std::to_string(v)); }
  void print(unsigned int v) { add(std::to_string(v)); }
  template <class T> void println(T v) { print(v); add("\n"); }
  void println() { add("\n"); }
};
extern SerialMock Serial;
