// =====================================================================
//  Osmonauts – OpenOsmosis Controller · configuration file
//  (c) 2021-2026 Kai Mommsen & Elias Dörre · SPDX-License-Identifier: CC-BY-NC-SA-4.0
//  Default configuration: Arduino Uno / Nano, 2 buttons, rain sensor, no display
//  Copy this file and adapt it to your hardware. All times in milliseconds,
//  use the conversion factors conv_s / conv_min / conv_h.
// =====================================================================

// ----------------------------- pin layout -----------------------------
#define relais1 4          // water supply valve
#define relais2 5          // filter process / tapping valve
#define relais3 6          // standing water valve
#define relais4 7          // membrane flush valve
#define pushbtnPIN1 2      // push button 1: tap water (short) / stop (long)
#define PUSHBTN2_ENABLE
#define pushbtnPIN2 3      // push button 2: stop (short)
//#define PUSHBTN_3_4_ENABLE
//#define pushbtnPIN3 10   // push button 3: disinfection (long)
//#define pushbtnPIN4 11   // push button 4: filter exchange (long)
#define tone_pin 9         // passive buzzer

// ----------------------------- options -----------------------------
//#define DEBUG            // print mode and sensor value once per second
//#define FEEDBACK         // LED / display feedback (requires LED and/or OLED_DISPLAY)
//#define LED              // WS2812B LEDs (Adafruit NeoPixel library)
//#define LEDpin 8
//#define NUMPIXEL 10
//#define OLED_DISPLAY     // SSD1306 128x32 I2C (Adafruit SSD1306 + GFX library)

#define OPERATION_NORMAL   // real durations
//#define OPERATION_TESTING  // short durations for testing on the bench

// ----------------------------- aqua stop -----------------------------
#define AQUASTOP_ENABLE
#define RAIN_SENSOR        // low value = wet
//#define WATER_LEVEL_SENSOR // high value = wet
const int     aquaStopThreshold    = 100;
const uint8_t aquaStopPin          = A0;
const uint8_t aquaStopTriggerCount = 5;   // consecutive wet readings until the controller stops

// ----------------------------- relais type -----------------------------
// active low relais board: ON = LOW, OFF = HIGH; active high relais board: reverse
#define ON  LOW
#define OFF HIGH

// ----------------------------- time conversion -----------------------------
const unsigned long ms       = 1000;
const unsigned long conv_s   = ms;
const unsigned long conv_min = ms * 60;
const unsigned long conv_h   = ms * 60 * 60;

#ifdef OPERATION_NORMAL
const uint8_t  buttonDebounceInterval = 100;   // [ms]
const uint16_t rst                    = 50;    // [ms] delay between switching two relais
const uint16_t intervalToLongPress    = 2000;  // [ms]

const unsigned long durFilterProcessShort       = 370 * conv_s;  // tapping duration
const unsigned long durMembraneFlushFirst       = 15  * conv_s;  // pre-flush membrane
const unsigned long durMembraneFlushSecond      = 20  * conv_s;  // post-flush membrane
const unsigned long durStandWaterFlush          = 90  * conv_s;  // pre-flush standing water
const unsigned long durMembraneFlushNewFilter   = 15  * conv_min;
const unsigned long durStandWaterFlushNewFilter = 15  * conv_min;

const unsigned long intervalStandWaterFlush   = 15 * conv_min;  // no new pre-flush within this time
const unsigned long intervalFlushAfterTapping = 3  * conv_min;  // wait before post-flush (several bottles)
const unsigned long intervalAutomaticFlush    = 6  * conv_h;
const unsigned long intervalAquaStopCheck     = 1  * conv_s;

const unsigned long intervalInjectionH2O2 = 2  * conv_min;  // time for one H2O2 injection
const unsigned long intervalMoveH2O2      = 8  * conv_s;    // approx. 150-200 ml (10 s = 140 ml)
const unsigned long intervalWaitDesinfect = 10 * conv_min;  // pause between pulses
const unsigned long intervalFinalFlushes  = 5  * conv_min;  // each final flush
const uint8_t       disinfectionPulses    = 7;
#endif

#ifdef OPERATION_TESTING
const uint8_t  buttonDebounceInterval = 100;
const uint16_t rst                    = 50;
const uint16_t intervalToLongPress    = 2000;

const unsigned long durFilterProcessShort       = 15 * conv_s;
const unsigned long durMembraneFlushFirst       = 5  * conv_s;
const unsigned long durMembraneFlushSecond      = 5  * conv_s;
const unsigned long durStandWaterFlush          = 5  * conv_s;
const unsigned long durMembraneFlushNewFilter   = 5  * conv_s;
const unsigned long durStandWaterFlushNewFilter = 5  * conv_s;

const unsigned long intervalStandWaterFlush   = 30 * conv_s;
const unsigned long intervalFlushAfterTapping = 10 * conv_s;
const unsigned long intervalAutomaticFlush    = 5  * conv_min;
const unsigned long intervalAquaStopCheck     = 1  * conv_s;

const unsigned long intervalInjectionH2O2 = 10 * conv_s;
const unsigned long intervalMoveH2O2      = 5  * conv_s;
const unsigned long intervalWaitDesinfect = 10 * conv_s;
const unsigned long intervalFinalFlushes  = 10 * conv_s;
const uint8_t       disinfectionPulses    = 3;
#endif
