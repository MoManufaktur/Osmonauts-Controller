#pragma once
/*
  Osmonauts – OpenOsmosis Controller · core: procedures, buttons, aqua stop, feedback
  Shared by both firmwares (source: firmware/common, copied into each sketch's src/ folder
  by tools/sync_common.py - edit only firmware/common):
    firmware/osmonauts_controller  Arduino Uno / Nano / Mega
    firmware/osmonauts_esp32       ESP32 4-relay board with WiFi, web interface, updates
  The sketch includes its config file first, then this file, and calls osmoSetup() / osmoLoop().

  Developers: Kai Mommsen & Elias Dörre
  Copyright (c) 2021-2026 Kai Mommsen & Elias Dörre
  SPDX-License-Identifier: CC-BY-NC-SA-4.0

  Change history
  --------------
  Created 06 03 2021
  By Kai Mommsen & Elias Dörre

  Modified 07 03 2021
  Kai Mommsen: adding relay switching time to reduce voltage peaks in electrical system

  Modified 08 03 2021
  Kai Mommsen changed relais state to
    - "HIGH" sets relais to NO = Normal Open (relais is deactivated and in its state conditions)--> electrical circuit to magnet valve is open --> in our case magnet valve is closed --> no water
    - "LOW" sets relais to NC = Normal Closed (relais is activated and in its operating conditions)--> electrical circuit to magnet valve is closed  --> in our case magnet valve is open --> running water
    - filter duration after installing new filters in total 30min --> membrane flush 15min and standing water flush 15min

  Modified 09 03 2021
  Elias Dörre: implenting further improvments of relais, define type of relais: 1. actice low relais: ON = LOW, OFF = HIGH, 2. active high relais reverse

  Modified 11 03 2021
  Kai Mommsen:
    - adding second membrane flush to clean membrane after tapping water or automatic flush procedure
    - implementing aquastop

  Modified 13 03 2021
  Elias Dörre:
    - Fix: Checked and corrected all if conditions and made them robust against the millis() overflow
    - Feature: Moved personal settings to configuration file
    - Integrated the second membrane flushing function into the first one

  Modified 14 03 2021
  Elias Dörre:
    - Fix: Replaced delay() with state machines and a wait() function
    - New Feature: Stop water tapping with a long button push
    - Fix: Stop program completly if aqua stop has tripped
    - Added simple debounce logic to aquaStop sensor
    - Added passive buzzer for acoustic feedback

  Modified 17 03 2021
  Elias Dörre: stop timer if long push detected

  Modified 23 03 2021
  Kai Mommsen: 4 buttons, pin mapping in config_kai, LED optical feedback
    (source: https://learn.adafruit.com/multi-tasking-the-arduino-part-3)

  Modified 27 03 2021
  Kai Mommsen: normal / testing operation in config file, time conversion factors h / min / s -> ms

  Modified 11 04 2021
  Kai Mommsen: merged first membrane flush and standing water flush, second membrane flush separated

  Modified 16 08 2021
  Elias Dörre: filter exchange procedure (long button 4 press)

  Modified 12 10 2021
  Elias Dörre: disinfection routine, relais switching in own functions

  Modified 22 03 2022
  Kai Mommsen: reducing memory usage for the OLED display library

  NEW INSIGHTS: disinfection mode requires an open valve on the output side during the
  injection of the hydrogen peroxide, so the pressure is able to release to normal air pressure

  Modified 02 01 2023
  Elias Dörre: config option intervalFlushAfterTapping (fill several bottles without flushing in between)

  Modified 04 02 2023
  Elias Dörre: prevent flushings between consecutive water tappings, "const" for constants

  Modified 16 11 2026  (version 6.1)
  Kai Mommsen & Elias Dörre:
    - ESP32 support (ESP32 4-relay board, config_esp32_relay4.h): WiFi setup portal, web interface,
      firmware update via web upload or from GitHub releases (SHA-256 checked, automatic rollback),
      optional daily update check that also counts active devices anonymously, local statistics
    - Filter change reminder: countdown (operating time, ESP32: calendar date), reset by the filter
      exchange procedure; OLED third line, LED ring orange pulse, optional light pin (FILTER_LED),
      three beeps after tapping when due, button 4 short = filter status
    - Arduino (Uno/Nano/Mega) and ESP32 are separate sketches sharing firmware/common
    - Fix: version 6.0 did not compile in the Arduino IDE (automatic function prototypes were inserted
      before the own data types) - all types moved to osmo_types.h

  Modified 15 11 2026  (version 6, code review assisted by Claude)
  Kai Mommsen & Elias Dörre:
    - Project renamed to "Osmonauts – OpenOsmosis Controller"
    - Fix: every procedure has its own timer (the single static timer in wait() was shared
      by all state machines and caused skipped or shortened steps)
    - Fix: central operating mode - only one procedure drives the relais at a time
    - Fix: stop really cancels the running procedure (before it only skipped one step)
    - Fix: disinfection now really waits between all steps (before: steps 4-13 ran in seconds)
    - Fix: post-flush interrupted by tapping no longer gets stuck
    - Fix: button debounce for press and release, long press measured on release
    - Fix: OLED status is no longer overwritten by the idle screen; OLED failure no longer halts the controller
    - Fix: all strings in flash (F()) - frees several hundred bytes of RAM for the OLED buffer
    - Config files cleaned up and completed, rst is now uint16_t
*/

#include <Arduino.h>
#define PROJECT_NAME "Osmonauts"
#define FW_VERSION   "6.1"
#define PROJECT_URL  "github.com/MoManufaktur/osmonauts-controller"

#include "osmo_types.h"     // data types
#ifdef OSMO_SIM
#undef OSMO_NET            // PC simulation (tests/): procedures only, no WiFi
#undef OSMO_CLOCK          // and no internet time - only the simulated time counts
#endif

// ============================== prototypes ==============================
void beep(int duration, int frequency = 1000);
void relaisMembraneFlush();
void relaisStandWaterFlush();
void relaisFilter();
void relaisInjection();
void stopRelais();

// relay state for status displays (bit 0 = relais1 ... bit 3 = relais4)
uint8_t relaisState = 0;
void relaisWrite(uint8_t pin, uint8_t value) {
  digitalWrite(pin, value);
  uint8_t bit = pin == relais1 ? 0 : pin == relais2 ? 1 : pin == relais3 ? 2 : 3;
  if (value == ON) relaisState |= (1 << bit); else relaisState &= ~(1 << bit);
}

#ifdef OSMO_NET
void netEvent(OsmoEvent e);     // ESP32: defined in osmo_esp32.h (statistics)
#else
inline void netEvent(OsmoEvent) {}
#endif

// true while a firmware update is running: no procedure may start
bool maintenanceLock = false;

// ============================== timer ==============================

// starts the timer on the first call, returns true once 'duration' [ms] has elapsed (millis() overflow safe)
bool timerDone(Timer &t, unsigned long duration) {
  if (!t.running) {
    t.start = millis();
    t.running = true;
    return false;
  }
  if (millis() - t.start >= duration) {
    t.running = false;
    return true;
  }
  return false;
}

void timerReset(Timer &t) {
  t.running = false;
}

// ============================== procedure helper ==============================

void procGoto(Proc &p, uint8_t state) {
  p.state = state;
  p.entered = false;
  timerReset(p.timer);
}

void procReset(Proc &p) {
  procGoto(p, 0);
}

// returns true exactly once when a step is entered -> place for switching relais, messages, beeps
bool procEntry(Proc &p) {
  if (!p.entered) {
    p.entered = true;
    return true;
  }
  return false;
}

// ============================== operating mode ==============================

Mode mode = MODE_IDLE;


void feedback(Feedback fb);
void feedbackUpdate();

// ============================== global variables ==============================
unsigned long msLastFlush           = 0;  // last membrane / standing water flush
unsigned long msLastAutomaticFlush  = 0;
unsigned long msLastWaterTapping    = 0;
unsigned long msLastAquaSensorCheck = 0;
unsigned long msLastAlarm           = 0;

bool forceFlush       = true;   // always pre-flush after power-on
bool postFlushPending = false;  // membrane post-flush due after tapping

uint8_t aquaStopCount = 0;
int     aquaStopValue = 0;
uint8_t disinfectionPulse = 0;

Proc preFlushProc;
Proc filterProc;
Proc postFlushProc;
Proc tapProc;
Proc autoFlushProc;
Proc filterChangeProc;
Proc disinfectionProc;

//////////////////////////// filter change reminder ////////////////////////////
// Counts the operating time since the last filter exchange (stored in EEPROM, saved once per hour).
// ESP32 with clock (OSMO_CLOCK, time from the internet): the calendar date counts instead.
// Reset automatically when the filter exchange procedure (button 4 long) has finished.
#include <EEPROM.h>
#ifdef OSMO_CLOCK
#include <time.h>
#endif
#ifndef FILTER_CHANGE_DAYS
#define FILTER_CHANGE_DAYS 180          // default: prefilters every 6 months
#endif
#ifndef EEPROM_ADDR
#define EEPROM_ADDR 0
#endif

struct FilterStore {
  uint16_t magic;
  uint16_t intervalDays;     // filter change interval [days]
  uint32_t minutes;          // operating minutes since the last filter change
  uint32_t lastChange;       // ESP32: date of the last filter change (unix time), 0 = unknown
};
const uint16_t FILTER_MAGIC = 0x0F17;
FilterStore filterStore;
unsigned long msFilterMinute = 0;
bool filterWasDue = false;

void filterSave() {
  EEPROM.put(EEPROM_ADDR, filterStore);   // AVR: only changed bytes are written
#ifdef ARDUINO_ARCH_ESP32
  EEPROM.commit();
#endif
}

#ifdef OSMO_CLOCK
bool clockValid() { return time(nullptr) > 1700000000; }
#endif

// minutes until the next filter change (negative = overdue)
long filterMinutesLeft() {
  long total = (long)filterStore.intervalDays * 1440L;
#ifdef OSMO_CLOCK
  if (clockValid() && filterStore.lastChange) return total - (long)((time(nullptr) - (time_t)filterStore.lastChange) / 60);
#endif
  return total - (long)filterStore.minutes;
}

bool filterDue() { return filterMinutesLeft() <= 0; }

int filterDaysLeft() {
  long m = filterMinutesLeft();
  return m <= 0 ? 0 : (int)((m + 1439) / 1440);
}

void filterPrintStatus() {
  if (filterDue()) {
    Serial.println(F("FILTER CHANGE DUE - exchange the filters, then long press button 4"));
  } else {
    Serial.print(F("Next filter change in "));
    Serial.print(filterDaysLeft());
    Serial.println(F(" days"));
  }
}

void filterLoad() {
#ifdef ARDUINO_ARCH_ESP32
  EEPROM.begin(64);
#endif
  EEPROM.get(EEPROM_ADDR, filterStore);
  if (filterStore.magic != FILTER_MAGIC) {          // first start: counter starts now
    filterStore.magic = FILTER_MAGIC;
    filterStore.intervalDays = FILTER_CHANGE_DAYS;
    filterStore.minutes = 0;
    filterStore.lastChange = 0;
    filterSave();
  }
#ifndef OSMO_NET
  if (filterStore.intervalDays != FILTER_CHANGE_DAYS) {   // Arduino: the config file sets the interval
    filterStore.intervalDays = FILTER_CHANGE_DAYS;
    filterSave();
  }
#endif
  msFilterMinute = millis();
  filterWasDue = filterDue();
}

void filterReset() {
  filterStore.minutes = 0;
  filterStore.lastChange = 0;
#ifdef OSMO_CLOCK
  if (clockValid()) filterStore.lastChange = (uint32_t)time(nullptr);
#endif
  filterSave();
  filterWasDue = false;
  Serial.println(F("Filter change reminder reset"));
  filterPrintStatus();
}

// called every loop: counts operating minutes, saves once per hour, reports when it becomes due
void filterTick() {
  if (millis() - msFilterMinute < 60000UL) return;
  msFilterMinute += 60000UL;
  filterStore.minutes++;
  bool save = (filterStore.minutes % 60) == 0;
#ifdef OSMO_CLOCK
  if (!filterStore.lastChange && clockValid()) {   // clock known now: date of the last change
    filterStore.lastChange = (uint32_t)(time(nullptr) - (time_t)filterStore.minutes * 60);
    save = true;
  }
#endif
  if (save) filterSave();
  bool due = filterDue();
  if (due && !filterWasDue) {
    filterPrintStatus();
    if (mode == MODE_IDLE) feedback(FB_IDLE);   // LED ring: reminder pattern
  }
  filterWasDue = due;
}

void filterReminderBeep() {
  for (uint8_t i = 0; i < 3; i++) { beep(60, 2000); delay(120); }
}

// ============================== buttons ==============================
#ifdef PUSHBTN2_ENABLE
  #define BTN2_PIN pushbtnPIN2
  #define BTN2_ON  true
#else
  #define BTN2_PIN 0
  #define BTN2_ON  false
#endif

#ifdef PUSHBTN_3_4_ENABLE
  #define BTN3_PIN pushbtnPIN3
  #define BTN4_PIN pushbtnPIN4
  #define BTN34_ON true
#else
  #define BTN3_PIN 0
  #define BTN4_PIN 0
  #define BTN34_ON false
#endif



Button buttons[4] = {
  { pushbtnPIN1, true,     HIGH, HIGH, 0, 0 },
  { BTN2_PIN,    BTN2_ON,  HIGH, HIGH, 0, 0 },
  { BTN3_PIN,    BTN34_ON, HIGH, HIGH, 0, 0 },
  { BTN4_PIN,    BTN34_ON, HIGH, HIGH, 0, 0 }
};

// debounced button read; the event (short/long) is reported when the button is released
ButtonEvent readButton(Button &b) {
  if (!b.enabled) return BTN_NONE;

  unsigned long now = millis();
  bool r = digitalRead(b.pin);

  if (r != b.reading) {          // raw signal changed -> restart debounce time
    b.reading = r;
    b.lastChange = now;
  }

  if (r != b.stable && now - b.lastChange >= buttonDebounceInterval) {
    b.stable = r;
    if (r == LOW) {
      b.pressedAt = now;
    } else {
      return (now - b.pressedAt >= intervalToLongPress) ? BTN_LONG : BTN_SHORT;
    }
  }
  return BTN_NONE;
}

//////////////////////////// LED optical feedback ////////////////////////////
#ifdef LED
#include "LED.h"

void RINGComplete();
NeoPatterns RING(NUMPIXEL, LEDpin, NEO_GRB + NEO_KHZ800, &RINGComplete);

void RINGComplete() {
  if (RING.ActivePattern == FADE || RING.ActivePattern == COLOR_WIPE) {
    RING.Reverse();
  }
}
#endif

//////////////////////////// OLED display ////////////////////////////
#ifdef OLED_DISPLAY
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH   128   // OLED display width, in pixels
#define SCREEN_HEIGHT  32    // OLED display height, in pixels
#define OLED_RESET     -1    // reset pin (-1 = sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  // 0x3C for 128x32, 0x3D for 128x64
// I2C pins: Uno/Nano A4 (SDA) A5 (SCL), Mega 20 (SDA) 21 (SCL)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool displayOk = false;
unsigned long msLastTimeUpdate = 0;
const unsigned long intervalTimeUpdate = 1000;
#endif

////////////////////////////////////////////////////////////////////////////////
void osmoSetup() {

  relaisWrite(relais1, OFF);  // prevents relais from activating after power-on
  relaisWrite(relais2, OFF);
  relaisWrite(relais3, OFF);
  relaisWrite(relais4, OFF);

  pinMode(relais1, OUTPUT);
  pinMode(relais2, OUTPUT);
  pinMode(relais3, OUTPUT);
  pinMode(relais4, OUTPUT);

  Serial.begin(115200);
  Serial.print(F("Start " PROJECT_NAME " v" FW_VERSION));
  Serial.println(F(" - OpenOsmosis Controller by Kai Mommsen & Elias Dörre"));
  Serial.println(F(PROJECT_URL " - CC BY-NC-SA 4.0, non-commercial use only"));

  filterLoad();
  filterPrintStatus();
#ifdef FILTER_LED
  pinMode(FILTER_LED, OUTPUT);
#endif

  for (uint8_t i = 0; i < 4; i++) {
    if (buttons[i].enabled) pinMode(buttons[i].pin, INPUT_PULLUP);
  }

#ifdef LED
  RING.begin();
  RING.setBrightness(20);  // about 1/5 (max = 255)
  RING.clear();
  RING.show();
#endif

#ifdef OLED_DISPLAY
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  displayOk = display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  if (!displayOk) {
    // the controller keeps working without display (before: endless loop = no aqua stop, no function)
    Serial.println(F("SSD1306 allocation failed - running without display"));
  } else {
    display.display();  // Adafruit splash screen
    delay(2000);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.setCursor(2, 2);
    display.println(F(PROJECT_NAME));
    display.setCursor(2, 12);
    display.println(F("OpenOsmosis Ctrl"));
    display.setCursor(2, 22);
    display.println(F("v" FW_VERSION " CC BY-NC-SA"));
    display.display();
    delay(3000);
  }
#endif

  feedback(FB_IDLE);
  beep(100);
  if (filterDue()) filterReminderBeep();
}

//////////////////////////// building blocks ////////////////////////////

// membrane flush followed by standing water flush (pre-flush)
// returns true when finished - or immediately if the last flush is less than intervalStandWaterFlush ago
bool preFlush(unsigned long durMembrane, unsigned long durStandWater) {
  Proc &p = preFlushProc;

  switch (p.state) {
    case 0:
      if (!forceFlush && millis() - msLastFlush < intervalStandWaterFlush) {
        Serial.println(F("  Membrane and standing water flush not necessary"));
        return true;
      }
      procGoto(p, 1);
      return false;

    case 1:
      if (procEntry(p)) {
        Serial.println(F("  Start membrane flush"));
        feedback(FB_MEMBRANE_FLUSH);
        relaisMembraneFlush();
      }
      if (timerDone(p.timer, durMembrane)) procGoto(p, 2);
      return false;

    case 2:
      if (procEntry(p)) {
        Serial.println(F("  Start standing water flush"));
        feedback(FB_STANDWATER_FLUSH);
        relaisStandWaterFlush();
      }
      if (timerDone(p.timer, durStandWater)) {
        stopRelais();
        msLastFlush = millis();
        forceFlush = false;
        procReset(p);
        return true;
      }
      return false;
  }
  procReset(p);
  return false;
}

// normal filter process = tapping water
bool filter(unsigned long duration) {
  Proc &p = filterProc;

  if (procEntry(p)) {
    Serial.println(F("  Start filtering"));
    feedback(FB_FILTER);
    relaisFilter();
  }
  if (timerDone(p.timer, duration)) {
    stopRelais();
    procReset(p);
    beep(1000);
    return true;
  }
  return false;
}

// second membrane flush (cleans the membrane after tapping or automatic flush)
bool postFlush() {
  Proc &p = postFlushProc;

  if (procEntry(p)) {
    Serial.println(F("  Start second membrane flush"));
    feedback(FB_POST_FLUSH);
    relaisMembraneFlush();
  }
  if (timerDone(p.timer, durMembraneFlushSecond)) {
    stopRelais();
    msLastFlush = millis();
    procReset(p);
    return true;
  }
  return false;
}

//////////////////////////// mode control ////////////////////////////

void resetAllProcs() {
  procReset(preFlushProc);
  procReset(filterProc);
  procReset(postFlushProc);
  procReset(tapProc);
  procReset(autoFlushProc);
  procReset(filterChangeProc);
  procReset(disinfectionProc);
  disinfectionPulse = 0;
}

void enterIdle() {
  resetAllProcs();
  mode = MODE_IDLE;
  feedback(FB_IDLE);
}

void startMode(Mode m) {
  resetAllProcs();
  mode = m;
}

// cancels the running procedure and closes all valves
void abortCurrentMode() {
  if (mode == MODE_IDLE || mode == MODE_AQUASTOP) return;

  Serial.println(F("Stop: current procedure cancelled"));
  netEvent(EV_STOP);

  if (mode == MODE_TAPPING && tapProc.state >= 2) {
    // water was already filtered -> membrane post-flush still makes sense
    postFlushPending = true;
    msLastWaterTapping = millis();
  }
  if (mode == MODE_POST_FLUSH) {
    postFlushPending = false;
  }
  if (mode == MODE_DISINFECTION) {
    Serial.println(F("WARNING: disinfection cancelled - hydrogen peroxide may still be in the system!"));
    forceFlush = true;
  }
  msLastAutomaticFlush = millis();  // otherwise an overdue automatic flush would restart immediately

  stopRelais();
  enterIdle();
}

// start a procedure on user request. Flushes (automatic / post) are interrupted,
// tapping, filter change and disinfection have to be stopped first.
void requestMode(Mode m) {
  if (mode == m) return;
  if (maintenanceLock) {
    Serial.println(F("Firmware update running - request refused"));
    return;
  }

  if (mode == MODE_AUTO_FLUSH || mode == MODE_POST_FLUSH) {
    abortCurrentMode();
  }

  if (mode == MODE_IDLE) {
    startMode(m);
  } else {
    Serial.println(F("Busy - stop the current procedure first"));
    beep(25); delay(100); beep(25); delay(100); beep(25);
  }
}

//////////////////////////// procedures ////////////////////////////

// tapping water: pre-flush (if necessary) -> filter -> post-flush later
void runTapping() {
  Proc &p = tapProc;

  switch (p.state) {
    case 0:
      Serial.println(F("Start water tapping procedure"));
      postFlushPending = false;
      procGoto(p, 1);
      break;

    case 1:
      if (preFlush(durMembraneFlushFirst, durStandWaterFlush)) procGoto(p, 2);
      break;

    case 2:
      if (filter(durFilterProcessShort)) procGoto(p, 3);
      break;

    case 3:
      stopRelais();
      msLastWaterTapping = millis();
      msLastFlush = millis();   // prevents flushing between consecutive water tappings (e.g. several bottles)
      postFlushPending = true;  // membrane post-flush after intervalFlushAfterTapping
      Serial.println(F("Water tapping procedure finished"));
      if (filterDue()) { filterPrintStatus(); filterReminderBeep(); }
      netEvent(EV_TAP);
      enterIdle();
      break;
  }
}

// post-flush of the membrane, started from idle after intervalFlushAfterTapping
void runPostFlush() {
  if (postFlush()) {
    netEvent(EV_POST_FLUSH);
    postFlushPending = false;
    msLastAutomaticFlush = millis();
    enterIdle();
  }
}

// automatic flush after intervalAutomaticFlush of inactivity
void runAutoFlush() {
  Proc &p = autoFlushProc;

  switch (p.state) {
    case 0:
      Serial.println(F("Start automatic flush procedure"));
      procGoto(p, 1);
      break;

    case 1:
      if (preFlush(durMembraneFlushFirst, durStandWaterFlush)) procGoto(p, 2);
      break;

    case 2:
      if (postFlush()) procGoto(p, 3);
      break;

    case 3:
      stopRelais();
      msLastAutomaticFlush = millis();
      Serial.println(F("Automatic flush procedure finished"));
      netEvent(EV_AUTO_FLUSH);
      enterIdle();
      break;
  }
}

// filter exchange: long flush after inserting new carbon / sediment cartridges or a new membrane
void runFilterChange() {
  Proc &p = filterChangeProc;

  switch (p.state) {
    case 0:
      Serial.println(F("Start filter exchange procedure"));
      feedback(FB_FILTER_CHANGE);
      forceFlush = true;
      procGoto(p, 1);
      break;

    case 1:
      if (preFlush(durMembraneFlushNewFilter, durStandWaterFlushNewFilter)) procGoto(p, 2);
      break;

    case 2:
      stopRelais();
      msLastAutomaticFlush = millis();
      Serial.println(F("Filter exchange procedure finished"));
      filterReset();
      netEvent(EV_FILTER_CHANGE);
      beep(1000);
      enterIdle();
      break;
  }
}

/*
  Disinfection procedure (start: long press button 3):

  1. Remove the plug from the upper one-way valve and insert a short hose piece (Schlauchstück)
  2. Start the disinfection program (short beep)
  3. Inject 20 ml hydrogen peroxide (3 %) with a disposable syringe (Einwegspritze).
     Valve 4 is open to release the pressure. Program continues after intervalInjectionH2O2.
  4. Standing water valve opens for intervalMoveH2O2 to move the H2O2 into the system
  5. Short beep: inject another 20 ml hydrogen peroxide, program continues after intervalInjectionH2O2
  6. The filter process is started for intervalMoveH2O2 and stopped for intervalWaitDesinfect,
     repeated disinfectionPulses times. This pushes the H2O2 slowly through the whole system.
  7. Final flushes: membrane, standing water and filter process, each intervalFinalFlushes
  8. Long beep: finished. The next tapping always starts with a pre-flush.
*/

void runDisinfection() {
  Proc &p = disinfectionProc;

  switch (p.state) {
    case DS_START:
      Serial.println(F("Start disinfection procedure"));
      feedback(FB_DISINFECTION);
      forceFlush = true;
      disinfectionPulse = 0;
      procGoto(p, DS_INJECT_1);
      break;

    case DS_INJECT_1:
    case DS_INJECT_2:
      if (procEntry(p)) {
        Serial.println(F("Please inject 20 ml hydrogen peroxide. Program continues automatically."));
        relaisInjection();
        beep(25);
      }
      if (timerDone(p.timer, intervalInjectionH2O2)) {
        procGoto(p, p.state == DS_INJECT_1 ? DS_MOVE : DS_PULSE);
      }
      break;

    case DS_MOVE:
      if (procEntry(p)) {
        Serial.println(F("Move hydrogen peroxide into the system"));
        beep(500);
        relaisStandWaterFlush();
      }
      if (timerDone(p.timer, intervalMoveH2O2)) {
        stopRelais();
        procGoto(p, DS_INJECT_2);
      }
      break;

    case DS_PULSE:
      if (procEntry(p)) {
        Serial.print(F("Disinfection pulse "));
        Serial.print(disinfectionPulse + 1);
        Serial.print(F(" of "));
        Serial.println(disinfectionPulses);
        beep(25);
        relaisFilter();
      }
      if (timerDone(p.timer, intervalMoveH2O2)) {
        stopRelais();
        procGoto(p, DS_PAUSE);
      }
      break;

    case DS_PAUSE:
      if (timerDone(p.timer, intervalWaitDesinfect)) {
        disinfectionPulse++;
        procGoto(p, disinfectionPulse < disinfectionPulses ? DS_PULSE : DS_FINAL_MEMBRANE);
      }
      break;

    case DS_FINAL_MEMBRANE:
      if (procEntry(p)) {
        Serial.println(F("Disinfection: final membrane flush"));
        relaisMembraneFlush();
      }
      if (timerDone(p.timer, intervalFinalFlushes)) procGoto(p, DS_FINAL_STANDWATER);
      break;

    case DS_FINAL_STANDWATER:
      if (procEntry(p)) {
        Serial.println(F("Disinfection: final standing water flush"));
        relaisStandWaterFlush();
      }
      if (timerDone(p.timer, intervalFinalFlushes)) procGoto(p, DS_FINAL_FILTER);
      break;

    case DS_FINAL_FILTER:
      if (procEntry(p)) {
        Serial.println(F("Disinfection: final filter flush"));
        relaisFilter();
      }
      if (timerDone(p.timer, intervalFinalFlushes)) procGoto(p, DS_DONE);
      break;

    case DS_DONE:
      stopRelais();
      msLastAutomaticFlush = millis();
      Serial.println(F("Disinfection procedure finished"));
      netEvent(EV_DISINFECTION);
      beep(1000);
      enterIdle();
      break;
  }
}

//////////////////////////// relais ////////////////////////////
// new valves are opened before the old ones are closed; rst spreads the switching
// to reduce current peaks

void relaisMembraneFlush() {
  Serial.println(F("  Open relais for membrane flush"));
  relaisWrite(relais1, ON);
  delay(rst);
  relaisWrite(relais4, ON);
  delay(rst);
  relaisWrite(relais2, OFF);
  delay(rst);
  relaisWrite(relais3, OFF);
}

void relaisStandWaterFlush() {
  Serial.println(F("  Open relais for standing water flush"));
  relaisWrite(relais1, ON);
  delay(rst);
  relaisWrite(relais3, ON);
  delay(rst);
  relaisWrite(relais2, OFF);
  delay(rst);
  relaisWrite(relais4, OFF);
}

void relaisFilter() {
  Serial.println(F("  Open relais for filtering"));
  relaisWrite(relais1, ON);
  delay(rst);
  relaisWrite(relais2, ON);
  delay(rst);
  relaisWrite(relais3, OFF);
  delay(rst);
  relaisWrite(relais4, OFF);
}

// disinfection: water supply closed, valve 4 open to release the pressure of the syringe
void relaisInjection() {
  Serial.println(F("  Open valve 4 to release pressure caused by the syringe"));
  relaisWrite(relais1, OFF);
  delay(rst);
  relaisWrite(relais2, OFF);
  delay(rst);
  relaisWrite(relais3, OFF);
  delay(rst);
  relaisWrite(relais4, ON);
}

void stopRelais() {
  Serial.println(F("  Stop all relais"));
  relaisWrite(relais1, OFF);  // water supply first
  delay(rst);
  relaisWrite(relais2, OFF);
  delay(rst);
  relaisWrite(relais3, OFF);
  delay(rst);
  relaisWrite(relais4, OFF);
}

//////////////////////////// aqua stop ////////////////////////////
#ifdef AQUASTOP_ENABLE
// reads the sensor every intervalAquaStopCheck; aquaStopTriggerCount consecutive "wet" readings
// close all valves and stop the controller until reset / power cycle
void checkAquaStop() {
  if (millis() - msLastAquaSensorCheck < intervalAquaStopCheck) return;
  msLastAquaSensorCheck = millis();

  aquaStopValue = analogRead(aquaStopPin);
  bool wet = false;

#ifdef RAIN_SENSOR
  if (aquaStopValue < aquaStopThreshold) wet = true;    // rain sensor: low value = wet
#endif
#ifdef WATER_LEVEL_SENSOR
  if (aquaStopValue > aquaStopThreshold) wet = true;    // water level sensor: high value = wet
#endif

  if (wet) {
    if (aquaStopCount < 255) aquaStopCount++;
  } else {
    aquaStopCount = 0;
  }

  if (aquaStopCount >= aquaStopTriggerCount && mode != MODE_AQUASTOP) {
    stopRelais();
    resetAllProcs();
    mode = MODE_AQUASTOP;
    feedback(FB_AQUASTOP);
    netEvent(EV_AQUASTOP);
    Serial.print(F("AQUA STOP triggered! Controller stopped until reset. Sensor value: "));
    Serial.println(aquaStopValue);
  }
}
#endif

// alarm while aqua stop is active: beep every 2 s
void aquaStopAlarm() {
  if (millis() - msLastAlarm >= 2000) {
    msLastAlarm = millis();
    Serial.print(F("AQUA STOP active, sensor value: "));
    Serial.println(aquaStopValue);
    beep(100);
    delay(150);
    beep(100);
  }
}

//////////////////////////// buttons ////////////////////////////

void onButton(uint8_t i, ButtonEvent ev) {
  switch (i) {
    case 0:  // button 1
      if (ev == BTN_SHORT) {
        Serial.println(F("Button 1: short press -> tap water"));
        beep(25);
        requestMode(MODE_TAPPING);
      } else {
        Serial.println(F("Button 1: long press -> stop"));
        beep(25); delay(150); beep(25);
        abortCurrentMode();
      }
      break;

    case 1:  // button 2
      if (ev == BTN_SHORT) {
        Serial.println(F("Button 2: short press -> stop"));
        beep(25); delay(150); beep(25);
        abortCurrentMode();
      }
      break;

    case 2:  // button 3
      if (ev == BTN_LONG) {
        Serial.println(F("Button 3: long press -> disinfection"));
        beep(100);
        requestMode(MODE_DISINFECTION);
      }
      break;

    case 3:  // button 4
      if (ev == BTN_LONG) {
        Serial.println(F("Button 4: long press -> filter exchange"));
        beep(25);
        requestMode(MODE_FILTER_CHANGE);
      } else {
        // short press: filter status - one long beep = ok, three short beeps = change due
        Serial.println(F("Button 4: short press -> filter status"));
        filterPrintStatus();
        if (filterDue()) filterReminderBeep(); else beep(400);
      }
      break;
  }
}

void handleButtons() {
  for (uint8_t i = 0; i < 4; i++) {
    ButtonEvent ev = readButton(buttons[i]);
    if (ev != BTN_NONE) onButton(i, ev);
  }
}

//////////////////////////// feedback LED / display ////////////////////////////
#ifdef FEEDBACK

void feedback(Feedback fb) {
#ifdef OLED_DISPLAY
  if (displayOk) {
    const __FlashStringHelper *txt = nullptr;
    switch (fb) {
      case FB_MEMBRANE_FLUSH:   txt = F("Membrane Flush");      break;
      case FB_POST_FLUSH:       txt = F("2nd Membrane Flush");  break;
      case FB_STANDWATER_FLUSH: txt = F("Stand Water Flush");   break;
      case FB_FILTER:           txt = F("Tapping Water");       break;
      case FB_FILTER_CHANGE:    txt = F("Exchange Filter");     break;
      case FB_DISINFECTION:     txt = F("Desinfection");        break;
      case FB_AQUASTOP:         txt = F("AQUA STOP!");          break;
      case FB_IDLE:             msLastTimeUpdate = millis() - intervalTimeUpdate; break;  // idle screen at once
    }
    if (txt != nullptr) {
      display.clearDisplay();
      display.setCursor(2, 2);
      display.println(txt);
      display.display();
    }
  }
#endif

#ifdef LED
  switch (fb) {
    case FB_IDLE:             // filter change due: slow orange pulse, otherwise off
                              if (filterDue()) RING.Fade(RING.Color(0, 0, 0), RING.Color(255, 110, 0), 40, 50);
                              else RING.ColorWipe(RING.Color(0, 0, 0), 50);
                              break;
    case FB_MEMBRANE_FLUSH:   RING.Fade(RING.Color(0, 0, 0), RING.Color(255, 170, 0), 20, 50);  break;
    case FB_POST_FLUSH:       RING.Fade(RING.Color(0, 0, 0), RING.Color(150, 50, 155), 20, 50); break;
    case FB_STANDWATER_FLUSH: RING.Fade(RING.Color(0, 0, 0), RING.Color(0, 170, 255), 20, 50);  break;
    case FB_FILTER:           RING.RainbowCycle(20);                                            break;
    case FB_FILTER_CHANGE:    RING.Fade(RING.Color(0, 0, 0), RING.Color(0, 255, 0), 20, 50);    break;
    case FB_DISINFECTION:     RING.Fade(RING.Color(0, 0, 0), RING.Color(255, 255, 255), 40, 50); break;
    case FB_AQUASTOP:         RING.ActivePattern = NONE; RING.ColorSet(RING.Color(255, 0, 0));  break;
  }
#endif
}

#ifdef OLED_DISPLAY
// idle screen: time until the next automatic flush
void showIdleScreen() {
  if (!displayOk || millis() - msLastTimeUpdate < intervalTimeUpdate) return;
  msLastTimeUpdate = millis();

  unsigned long elapsed = millis() - msLastAutomaticFlush;
  unsigned long tme = (elapsed >= intervalAutomaticFlush) ? 0 : (intervalAutomaticFlush - elapsed) / 1000;  // [s]
  unsigned long hr   = tme / 3600;
  unsigned long mins = (tme % 3600) / 60;
  unsigned long sec  = tme % 60;

  display.clearDisplay();
  display.setCursor(2, 2);
  display.print(F("Next Automatic Flush"));
  display.setCursor(2, 12);
  display.print(hr);
  display.print(F(" h  "));
  display.print(mins);
  display.print(F(" min  "));
  display.print(sec);
  display.print(F(" s"));

  // third line: filter change countdown, blinking inverted when due
  display.setCursor(2, 22);
  if (filterDue()) {
    bool on = (millis() / 1000) % 2;
    if (on) display.fillRect(0, 20, SCREEN_WIDTH, 11, SSD1306_WHITE);
    display.setTextColor(on ? SSD1306_BLACK : SSD1306_WHITE);
    display.print(F("FILTER CHANGE DUE!"));
    display.setTextColor(SSD1306_WHITE);
  } else {
    display.print(F("Filter in "));        // 128 px = 21 characters per line
    display.print(filterDaysLeft());
    display.print(F(" d"));
  }
  display.display();
}
#endif

void feedbackUpdate() {
#ifdef LED
  RING.Update();
#endif
#ifdef OLED_DISPLAY
  if (mode == MODE_IDLE) showIdleScreen();
#endif
}

#else
void feedback(Feedback) {}
void feedbackUpdate() {}
#endif

//////////////////////////// buzzer ////////////////////////////
void beep(int duration, int frequency) {
  tone(tone_pin, frequency);
  delay(duration);
  noTone(tone_pin);
}

//////////////////////////// main loop ////////////////////////////
void osmoLoop() {

  filterTick();
#ifdef FILTER_LED
  // light signal: blinks once per second while the filter change is due
  digitalWrite(FILTER_LED, filterDue() && (millis() / 500) % 2 ? HIGH : LOW);
#endif

#ifdef DEBUG
  static unsigned long msLastDebug = 0;
  if (millis() - msLastDebug >= 1000) {
    msLastDebug = millis();
    Serial.print(F("Mode: "));
    Serial.print((int)mode);
    Serial.print(F("  AquaStopValue: "));
    Serial.println(aquaStopValue);
  }
#endif

  feedbackUpdate();

#ifdef AQUASTOP_ENABLE
  checkAquaStop();
#endif

  // aqua stop: everything stays off until reset
  if (mode == MODE_AQUASTOP) {
    aquaStopAlarm();
    return;
  }

  handleButtons();

  switch (mode) {
    case MODE_TAPPING:       runTapping();      break;
    case MODE_POST_FLUSH:    runPostFlush();    break;
    case MODE_AUTO_FLUSH:    runAutoFlush();    break;
    case MODE_FILTER_CHANGE: runFilterChange(); break;
    case MODE_DISINFECTION:  runDisinfection(); break;

    case MODE_IDLE:
      if (maintenanceLock) break;
      if (postFlushPending && millis() - msLastWaterTapping >= intervalFlushAfterTapping) {
        startMode(MODE_POST_FLUSH);
      } else if (millis() - msLastAutomaticFlush >= intervalAutomaticFlush) {
        startMode(MODE_AUTO_FLUSH);
      }
      break;

    default:
      break;
  }
}
