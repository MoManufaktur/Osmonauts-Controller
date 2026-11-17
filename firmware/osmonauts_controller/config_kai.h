// =====================================================================
//  Osmonauts – OpenOsmosis Controller · configuration file
//  (c) 2021-2026 Kai Mommsen & Elias Dörre · SPDX-License-Identifier: CC-BY-NC-SA-4.0
//  Configuration Kai: Arduino Mega 2560, 4 buttons, water level sensor,
//  active low relais, WS2812B LED ring and SSD1306 OLED (I2C: SDA 20, SCL 21)
//
//  Pin mapping Mega:
//   22 relais1   23 button 1
//   24 relais2   25 button 2
//   26 relais3   27 button 3
//   28 relais4   29 button 4
//   30 buzzer    31 LED data
//   A0 aqua stop sensor
//
//  Uno variant (same functions): relais 4-7, buttons 2/3/10/11, buzzer 8, LED 9,
//  OLED on A4/A5 - note the Uno has only 2 KB RAM, the OLED needs 512 bytes of it.
// =====================================================================

// ----------------------------- pin layout -----------------------------
#define relais1 22         // water supply valve
#define relais2 24         // filter process / tapping valve
#define relais3 26         // standing water valve
#define relais4 28         // membrane flush valve
#define pushbtnPIN1 23     // push button 1: tap water (short) / stop (long)
#define PUSHBTN2_ENABLE
#define pushbtnPIN2 25     // push button 2: stop (short)
#define PUSHBTN_3_4_ENABLE
#define pushbtnPIN3 27     // push button 3: disinfection (long)
#define pushbtnPIN4 29     // push button 4: filter exchange (long)
#define tone_pin 30        // passive buzzer
#define LEDpin 31          // data pin of the LED strip / ring

// ----------------------------- options -----------------------------
//#define DEBUG
#define FEEDBACK           // LED and display feedback

#define OPERATION_NORMAL
//#define OPERATION_TESTING

#define LED                // WS2812B LEDs (Adafruit NeoPixel library)
#define NUMPIXEL 10        // number of LEDs on the strip or ring
#define OLED_DISPLAY       // SSD1306 128x32

// ----------------------------- filter change reminder -----------------------------
#define FILTER_CHANGE_DAYS 180     // interval [days] - counts operating time, reset by the filter exchange procedure
#define FILTER_LED LED_BUILTIN     // light signal: on-board LED (D13) blinks when the filter change is due

// ----------------------------- aqua stop -----------------------------
#define AQUASTOP_ENABLE
//#define RAIN_SENSOR
#define WATER_LEVEL_SENSOR
const int     aquaStopThreshold    = 50;
const uint8_t aquaStopPin          = A0;
const uint8_t aquaStopTriggerCount = 5;

// ----------------------------- relais type -----------------------------
#define ON  LOW
#define OFF HIGH

// ----------------------------- time conversion -----------------------------
const unsigned long ms       = 1000;
const unsigned long conv_s   = ms;
const unsigned long conv_min = ms * 60;
const unsigned long conv_h   = ms * 60 * 60;

#ifdef OPERATION_NORMAL
const uint8_t  buttonDebounceInterval = 100;   // [ms]
const uint16_t rst                    = 500;   // [ms] (was uint8_t -> overflow to 244 ms)
const uint16_t intervalToLongPress    = 2000;  // [ms]

const unsigned long durFilterProcessShort       = 6  * conv_min;
const unsigned long durMembraneFlushFirst       = 15 * conv_s;
const unsigned long durMembraneFlushSecond      = 20 * conv_s;
const unsigned long durStandWaterFlush          = 90 * conv_s;
const unsigned long durMembraneFlushNewFilter   = 15 * conv_min;
const unsigned long durStandWaterFlushNewFilter = 15 * conv_min;

const unsigned long intervalStandWaterFlush   = 15 * conv_min;
const unsigned long intervalFlushAfterTapping = 1  * conv_min;
const unsigned long intervalAutomaticFlush    = 6  * conv_h;
const unsigned long intervalAquaStopCheck     = 1  * conv_s;

const unsigned long intervalInjectionH2O2 = 2  * conv_min;
const unsigned long intervalMoveH2O2      = 8  * conv_s;
const unsigned long intervalWaitDesinfect = 10 * conv_min;
const unsigned long intervalFinalFlushes  = 5  * conv_min;
const uint8_t       disinfectionPulses    = 7;
#endif

#ifdef OPERATION_TESTING
const uint8_t  buttonDebounceInterval = 100;
const uint16_t rst                    = 500;
const uint16_t intervalToLongPress    = 2000;

const unsigned long durFilterProcessShort       = 15 * conv_s;
const unsigned long durMembraneFlushFirst       = 5  * conv_s;
const unsigned long durMembraneFlushSecond      = 5  * conv_s;
const unsigned long durStandWaterFlush          = 5  * conv_s;
const unsigned long durMembraneFlushNewFilter   = 5  * conv_s;
const unsigned long durStandWaterFlushNewFilter = 5  * conv_s;

const unsigned long intervalStandWaterFlush   = 5 * conv_s;
const unsigned long intervalFlushAfterTapping = 1 * conv_min;
const unsigned long intervalAutomaticFlush    = 5 * conv_min;
const unsigned long intervalAquaStopCheck     = 1 * conv_s;

const unsigned long intervalInjectionH2O2 = 10 * conv_s;
const unsigned long intervalMoveH2O2      = 10 * conv_s;
const unsigned long intervalWaitDesinfect = 10 * conv_s;
const unsigned long intervalFinalFlushes  = 10 * conv_s;
const uint8_t       disinfectionPulses    = 3;
#endif
