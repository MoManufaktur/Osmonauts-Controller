// =====================================================================
//  Osmonauts – OpenOsmosis Controller · configuration file
//  (c) 2021-2026 Kai Mommsen & Elias Dörre · SPDX-License-Identifier: CC-BY-NC-SA-4.0
//  Configuration Elias: Arduino Nano (ATmega328P), buttons 1/3/4,
//  rain sensor on A7, active high relais, no display
//  Note: A6/A7 only exist on Nano / Pro Mini / SMD boards, not on a DIP Uno!
// =====================================================================

// ----------------------------- pin layout -----------------------------
#define relais1 4          // water supply valve
#define relais2 5          // filter process / tapping valve
#define relais3 6          // standing water valve
#define relais4 7          // membrane flush valve
#define pushbtnPIN1 3      // push button 1: tap water (short) / stop (long)
//#define PUSHBTN2_ENABLE
//#define pushbtnPIN2 12   // push button 2: stop (short)
#define PUSHBTN_3_4_ENABLE
#define pushbtnPIN3 11     // push button 3: disinfection (long)
#define pushbtnPIN4 2      // push button 4: filter exchange (long)
#define tone_pin 9         // passive buzzer

// ----------------------------- options -----------------------------
//#define DEBUG
//#define FEEDBACK

#define OPERATION_NORMAL
//#define OPERATION_TESTING

// ----------------------------- aqua stop -----------------------------
#define AQUASTOP_ENABLE
#define RAIN_SENSOR
//#define WATER_LEVEL_SENSOR
const int     aquaStopThreshold    = 150;
const uint8_t aquaStopPin          = A7;
const uint8_t aquaStopTriggerCount = 5;

// ----------------------------- relais type -----------------------------
#define ON  HIGH
#define OFF LOW

// ----------------------------- time conversion -----------------------------
const unsigned long ms       = 1000;
const unsigned long conv_s   = ms;
const unsigned long conv_min = ms * 60;
const unsigned long conv_h   = ms * 60 * 60;

#ifdef OPERATION_NORMAL
const uint8_t  buttonDebounceInterval = 100;   // [ms]
const uint16_t rst                    = 200;   // [ms]
const uint16_t intervalToLongPress    = 1500;  // [ms]

const unsigned long durFilterProcessShort       = 215 * conv_s;
const unsigned long durMembraneFlushFirst       = 10  * conv_s;
const unsigned long durMembraneFlushSecond      = 15  * conv_s;
const unsigned long durStandWaterFlush          = 90  * conv_s;
const unsigned long durMembraneFlushNewFilter   = 15  * conv_min;
const unsigned long durStandWaterFlushNewFilter = 15  * conv_min;

const unsigned long intervalStandWaterFlush   = 15 * conv_min;
const unsigned long intervalFlushAfterTapping = 3  * conv_min;
const unsigned long intervalAutomaticFlush    = 6  * conv_h;
const unsigned long intervalAquaStopCheck     = 1  * conv_s;

const unsigned long intervalInjectionH2O2 = 2  * conv_min;
const unsigned long intervalMoveH2O2      = 8  * conv_s;    // 10 s = 140 ml
const unsigned long intervalWaitDesinfect = 10 * conv_min;
const unsigned long intervalFinalFlushes  = 5  * conv_min;
const uint8_t       disinfectionPulses    = 7;
#endif

#ifdef OPERATION_TESTING
const uint8_t  buttonDebounceInterval = 100;
const uint16_t rst                    = 50;
const uint16_t intervalToLongPress    = 2000;

const unsigned long durFilterProcessShort       = 6 * conv_s;
const unsigned long durMembraneFlushFirst       = 5 * conv_s;
const unsigned long durMembraneFlushSecond      = 5 * conv_s;
const unsigned long durStandWaterFlush          = 5 * conv_s;
const unsigned long durMembraneFlushNewFilter   = 5 * conv_s;
const unsigned long durStandWaterFlushNewFilter = 5 * conv_s;

const unsigned long intervalStandWaterFlush   = 5  * conv_s;
const unsigned long intervalFlushAfterTapping = 10 * conv_s;
const unsigned long intervalAutomaticFlush    = 10 * conv_min;
const unsigned long intervalAquaStopCheck     = 1  * conv_s;

const unsigned long intervalInjectionH2O2 = 10 * conv_s;
const unsigned long intervalMoveH2O2      = 5  * conv_s;
const unsigned long intervalWaitDesinfect = 10 * conv_s;
const unsigned long intervalFinalFlushes  = 10 * conv_s;
const uint8_t       disinfectionPulses    = 3;
#endif
