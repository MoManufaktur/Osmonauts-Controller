#pragma once
// Osmonauts – OpenOsmosis Controller · data types
// (c) 2021-2026 Kai Mommsen & Elias Dörre · SPDX-License-Identifier: CC-BY-NC-SA-4.0
//
// All types live in this header, included before the first function of the sketch:
// the Arduino IDE inserts automatic function prototypes before the first function,
// so every type used in a function signature has to be known at that point.

#include <Arduino.h>

// Non-blocking timer. Every procedure owns its own timer, so procedures never disturb each other.
struct Timer {
  unsigned long start;
  bool running;
};

// state of one state machine: current step, entry flag (step actions run once) and own timer
struct Proc {
  uint8_t state;
  bool entered;
  Timer timer;
};

enum Mode : uint8_t {
  MODE_IDLE,
  MODE_TAPPING,
  MODE_AUTO_FLUSH,
  MODE_POST_FLUSH,
  MODE_FILTER_CHANGE,
  MODE_DISINFECTION,
  MODE_AQUASTOP
};

// feedback states for LED and display
enum Feedback : uint8_t {
  FB_IDLE,
  FB_MEMBRANE_FLUSH,
  FB_POST_FLUSH,
  FB_STANDWATER_FLUSH,
  FB_FILTER,
  FB_FILTER_CHANGE,
  FB_DISINFECTION,
  FB_AQUASTOP
};

enum ButtonEvent : uint8_t { BTN_NONE, BTN_SHORT, BTN_LONG };

struct Button {
  uint8_t pin;
  bool enabled;
  bool stable;               // debounced state (HIGH = released)
  bool reading;              // last raw reading
  unsigned long lastChange;  // time of last raw change
  unsigned long pressedAt;   // time of debounced press
};

enum DisinfectionStep : uint8_t {
  DS_START, DS_INJECT_1, DS_MOVE, DS_INJECT_2, DS_PULSE, DS_PAUSE,
  DS_FINAL_MEMBRANE, DS_FINAL_STANDWATER, DS_FINAL_FILTER, DS_DONE
};

// events for statistics (counted on the ESP32, ignored on Uno / Mega)
enum OsmoEvent : uint8_t { EV_TAP, EV_POST_FLUSH, EV_AUTO_FLUSH, EV_FILTER_CHANGE, EV_DISINFECTION, EV_AQUASTOP, EV_STOP };
