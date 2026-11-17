/*
  Osmonauts – OpenOsmosis Controller · ESP32 version (ESP32 4-relay board)
  Controller for DIY reverse osmosis systems with WiFi, web interface and firmware updates
  https://github.com/MoManufaktur/Osmonauts-Controller

  Developers: Kai Mommsen & Elias Dörre
  Copyright (c) 2021-2026 Kai Mommsen & Elias Dörre

  License: Creative Commons Attribution-NonCommercial-ShareAlike 4.0 (CC BY-NC-SA 4.0)
    - free for private, non-commercial use
    - modifications allowed; shared modifications must use the same license
    - keep this notice and the start message (name, authors, link) in your version
    - commercial use only with written permission of the authors
  SPDX-License-Identifier: CC-BY-NC-SA-4.0

  Same procedures as the Arduino version (src/osmo_core.h, shared), plus:
      WiFi setup via hotspot "Osmonauts-XXXX" (WiFiManager)
      Web interface http://osmonauts.local - status, stop, flushes, filter reminder with date, settings
      Firmware update by upload or from the latest GitHub release (SHA-256 checked, automatic rollback)
      Optional daily update check - GitHub counts it as an anonymous active device
  Safety: procedures, buttons and aqua stop do not depend on WiFi. Tapping only at the device.

  Arduino IDE: board "ESP32 Dev Module", partition scheme "Minimal SPIFFS (1.9MB APP with OTA)"
  Libraries: WiFiManager (tzapu), ESP Async WebServer + Async TCP (ESP32Async)
  Details: README.md in this folder.
*/

#include "config_esp32_relay4.h"   // pins and durations of the ESP32 4-relay board

#include "src/osmo_core.h"         // procedures, buttons, aqua stop, filter reminder, feedback
#include "osmo_esp32.h"            // WiFi, web interface, updates, statistics

void setup() {
  osmoSetup();
  netSetup();
}

void loop() {
  netLoop();    // WiFi, web commands, update check - also while aqua stop is active
  osmoLoop();
}
