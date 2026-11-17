// =====================================================================
//  Osmonauts – OpenOsmosis Controller · configuration file
//  (c) 2021-2026 Kai Mommsen & Elias Dörre · SPDX-License-Identifier: CC-BY-NC-SA-4.0
//
//  ESP32 4-relay board (ESP32-WROOM-32E, "ESP32 Relay X4", AC 90-250 V / DC 7-30 V / 5 V)
//  Relays on GPIO 32 / 33 / 25 / 26 (active HIGH), status LED GPIO 23, BOOT button GPIO 0
//
//  Arduino IDE: board "ESP32 Dev Module", Partition Scheme "Minimal SPIFFS (1.9MB APP with OTA)"
//  First upload with a 3.3 V USB-serial adapter (TX, RX, GND, 5 V) - hold the button on GPIO 0
//  while powering up. All later updates via the web interface (WiFi).
//
//  Libraries: WiFiManager (tzapu), ESPAsyncWebServer + AsyncTCP (ESP32Async)
// =====================================================================

// ----------------------------- board / network -----------------------------
#define OSMO_NET                          // WiFi, web interface, firmware update
#define BOARD_VARIANT "esp32-relay4"      // name of the firmware and ping file in the GitHub release
#define GITHUB_REPO   "MoManufaktur/Osmonauts-Controller"
#define STATUS_LED     23                 // on-board LED: on = procedure running, fast blink = WiFi setup,
                                          //   slow blink = aqua stop, double flash = filter change due
#define WIFI_RESET_PIN 0                  // BOOT button: hold 5 s = delete WiFi settings and restart

// ----------------------------- pin layout -----------------------------
// Pins on the double row headers. Do not use GPIO 0, 2, 12, 15 (boot) and 6-11 (flash).
#define relais1 32         // water supply valve        (relay 1 on the board)
#define relais2 33         // filter process / tapping  (relay 2)
#define relais3 25         // standing water valve      (relay 3)
#define relais4 26         // membrane flush valve      (relay 4)
#define pushbtnPIN1 16     // push button 1: tap water (short) / stop (long)
#define PUSHBTN2_ENABLE
#define pushbtnPIN2 17     // push button 2: stop (short)
#define PUSHBTN_3_4_ENABLE
#define pushbtnPIN3 18     // push button 3: disinfection (long)
#define pushbtnPIN4 19     // push button 4: filter exchange (long)
#define tone_pin 27        // passive buzzer

// ----------------------------- options -----------------------------
//#define DEBUG
//#define FEEDBACK         // LED ring / OLED (optional)
//#define LED
//#define LEDpin 13
//#define NUMPIXEL 10
//#define OLED_DISPLAY     // SSD1306 128x32 on SDA 21 / SCL 22

#define OPERATION_NORMAL
//#define OPERATION_TESTING

// ----------------------------- filter change reminder -----------------------------
#define FILTER_CHANGE_DAYS 180     // default interval [days], changeable in the web interface
#define OSMO_CLOCK                 // date from the internet (NTP): reminder counts calendar days
// light signal: the on-board status LED double-flashes in idle mode while the filter change is due

// ----------------------------- aqua stop -----------------------------
// ESP32: analog input only on ADC1 (GPIO 32-39) while WiFi is active. GPIO 34-39 are input only
// and have no pull-ups. ADC range 0-4095 (12 bit) - measure dry/wet values with DEBUG.
#define AQUASTOP_ENABLE
#define RAIN_SENSOR        // low value = wet
//#define WATER_LEVEL_SENSOR // high value = wet
const int     aquaStopThreshold    = 600;
const uint8_t aquaStopPin          = 34;
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
const unsigned long intervalMoveH2O2      = 8  * conv_s;
const unsigned long intervalWaitDesinfect = 10 * conv_min;
const unsigned long intervalFinalFlushes  = 5  * conv_min;
const uint8_t       disinfectionPulses    = 7;
#endif

#ifdef OPERATION_TESTING
const uint8_t  buttonDebounceInterval = 100;
const uint16_t rst                    = 50;
const uint16_t intervalToLongPress    = 1500;

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
