/*
  Osmonauts – OpenOsmosis Controller · Arduino version (Uno / Nano / Mega)
  Controller for DIY reverse osmosis systems
  https://github.com/MoManufaktur/Osmonauts-Controller

  Developers: Kai Mommsen & Elias Dörre
  Copyright (c) 2021-2026 Kai Mommsen & Elias Dörre

  License: Creative Commons Attribution-NonCommercial-ShareAlike 4.0 (CC BY-NC-SA 4.0)
    - free for private, non-commercial use
    - modifications allowed; shared modifications must use the same license
    - keep this notice and the start message (name, authors, link) in your version
    - commercial use only with written permission of the authors
  SPDX-License-Identifier: CC-BY-NC-SA-4.0

  Features:
      Automatic pre-flush (membrane + standing water) before tapping
      Delayed post-flush of the membrane after tapping (fill several bottles in a row)
      Automatic flush after several hours of inactivity (keeps the system as germ-free as possible)
      Filter exchange procedure (long flush after inserting new cartridges)
      Disinfection routine with hydrogen peroxide (H2O2)
      Aqua stop sensor (rain sensor or water level sensor) - latches until reset
      Stop any running procedure with a long push on button 1 or a short push on button 2
      Acoustic feedback with passive buzzer
      Optional LED (WS2812B) and I2C OLED feedback
      Filter change reminder: countdown on OLED / LED, light signal and beeps when due

  The circuit (pins are defined in the config file, values below = config.h):
      Push button 1..4 against GND (internal pull-up, no external resistor needed)
      Relais 1  water supply valve
      Relais 2  filter / tapping valve
      Relais 3  standing water valve
      Relais 4  membrane flush valve
      Analog input for aqua stop sensor
      Passive buzzer

  Button mapping:
      Button 1 short : tap water
      Button 1 long  : stop current procedure
      Button 2 short : stop current procedure (optional button)
      Button 3 long  : disinfection routine
      Button 4 long  : filter exchange procedure

  The procedures live in src/osmo_core.h (shared with the ESP32 version, change history there).
  ESP32 4-relay board with WiFi and web interface: see firmware/osmonauts_esp32.
*/

// ============================== configuration ==============================
// Choose ONE config file:
#include "config.h"          // default: Arduino Uno / Nano, no display
//#include "config_elias.h"  // Elias: Nano, rain sensor on A7, 3 buttons
//#include "config_kai.h"    // Kai: Arduino Mega, LED ring + OLED

#include "src/osmo_core.h"   // procedures, buttons, aqua stop, filter reminder, feedback

void setup() {
  osmoSetup();
}

void loop() {
  osmoLoop();
}
