# Authors – Osmonauts · OpenOsmosis Controller

Copyright © 2021–2026 Kai Mommsen & Elias Dörre · licensed under CC BY-NC-SA 4.0 (see LICENSE).
Commercial licenses can only be granted by both authors together.

## Developers

**Kai Mommsen**
- Relay switching logic and relay timing (voltage peaks)
- Aqua stop, second membrane flush, button mapping, Arduino Mega port
- LED ring and OLED feedback, normal / testing operation, time conversion in the config files

**Elias Dörre**
- Non-blocking architecture: state machines instead of `delay()`
- Configuration file system, millis() overflow safe timing
- Stop by long button push, debounce of the aqua stop sensor, buzzer feedback
- Filter exchange procedure, disinfection routine, relay functions
- Waiting time before post-flush (several bottles in a row), "const" optimisation

Version 6 (2026) was created by Kai Mommsen & Elias Dörre after a code review assisted by Claude (Anthropic).

## Third-party code

- `LED.h` – based on "Multi-tasking the Arduino – Part 3" by Bill Earl, Adafruit Industries
- `docs/osmonauts_spuelvorgaenge_3d.html` – embeds three.js r158 (MIT License, © three.js authors)
- ESP32 version (linked as libraries, not included in this repository):
  - WiFiManager by tzapu – MIT License
  - ESPAsyncWebServer and AsyncTCP by the ESP32Async organisation – LGPL-3.0.
    The complete source code of the Osmonauts firmware is public, so everybody can rebuild the
    firmware with a modified version of these libraries (LGPL-3.0 section 4).
  - Arduino core for the ESP32 by Espressif – LGPL-2.1
