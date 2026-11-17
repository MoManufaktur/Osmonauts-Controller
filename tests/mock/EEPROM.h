// Osmonauts – host-side mock of the Arduino EEPROM library for simulation tests
// (c) 2026 Kai Mommsen & Elias Dörre · SPDX-License-Identifier: CC-BY-NC-SA-4.0
#pragma once
#include <cstdint>
#include <cstring>
struct EEPROMClass {
  uint8_t data[4096];
  int writes = 0;
  EEPROMClass() { memset(data, 0xFF, sizeof data); }   // erased EEPROM
  template <class T> T &get(int a, T &t) { memcpy(&t, data + a, sizeof(T)); return t; }
  template <class T> const T &put(int a, const T &t) { memcpy(data + a, &t, sizeof(T)); writes++; return t; }
  void begin(size_t) {}
  bool commit() { return true; }
};
extern EEPROMClass EEPROM;
