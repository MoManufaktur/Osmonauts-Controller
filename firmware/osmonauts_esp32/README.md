# Osmonauts – OpenOsmosis Controller · ESP32 version

Firmware for the **ESP32 4-relay board** (ESP32-WROOM-32E, 4 relays and power supply on board,
AC 90–250 V / DC 7–30 V / 5 V, approx. 10–15 €). Same procedures as the Arduino version – the core in `src/`
is shared (source: [`firmware/common`](../common)) – plus WiFi, web interface, updates and the filter reminder with date.

by **Kai Mommsen & Elias Dörre** · CC BY-NC-SA 4.0 · [main README](../../README.md)


The ESP32 4-relay board runs the same procedures plus a network module. **Everything that keeps the system
safe works without WiFi**: buttons, all procedures and the aqua stop run exactly as on the Arduino.

**First installation**
1. The board has no USB port: connect a 3.3 V USB-serial adapter to the programming header (TX→RX, RX→TX, GND, 5 V).
2. Either flash the ready image `osmonauts-esp32-relay4-full.bin` from the latest [release](https://github.com/MoManufaktur/Osmonauts-Controller/releases) at address `0x0`
   (e.g. `esptool.py write_flash 0x0 osmonauts-esp32-relay4-full.bin`),
   or open `osmonauts_esp32.ino` in the Arduino IDE: board *ESP32 Dev Module*, partition scheme
   **Minimal SPIFFS (1.9MB APP with OTA)**, libraries *WiFiManager*, *ESP Async WebServer*, *Async TCP*.
   Hold the button on GPIO 0 while powering up to enter the upload mode.

**WiFi setup**
1. After the first start the controller opens the hotspot **`Osmonauts-XXXX`**. Connect with your phone; the setup page opens
   (otherwise browse to `http://192.168.4.1`).
2. Choose your WiFi, enter the password, set a password for the web interface and decide about the daily update check.
3. The controller connects and is reachable at **http://osmonauts.local** (or its IP address). User: `admin`.
   The web password is also printed on the serial monitor at every start.
4. To change the WiFi: hold the BOOT button (GPIO 0) for 5 seconds, or use *WLAN zurücksetzen* in the web interface.

**Web interface** – current step with remaining time, valve states, aqua stop sensor value, next automatic flush,
date and time, buttons for *Stop*, *Flush now*, *Filter exchange* and *Disinfection*, local statistics since the first
start, firmware update and settings.

**Filter change reminder** – card *Filterwechsel*: days left, date of the next and the last filter change,
interval (adjustable, 7–1095 days, default 180). The countdown counts calendar days as soon as the controller has the
time from the internet. When due: the card turns orange, the status LED double-flashes, three beeps after tapping,
the OLED (if connected) shows `FILTER CHANGE DUE!`. The filter exchange procedure or the button
*Filter gewechselt – Countdown neu starten* restarts the countdown.
Tapping water can only be started at the device (button 1) – no water runs because of a remote click.

**Firmware updates** – only in idle mode with all valves closed:
- *Search for update* loads the update manifest of the latest GitHub release; *Install update* downloads the firmware
  via HTTPS, checks its SHA-256 checksum and installs it. Optional: install updates automatically in idle mode.
- Or upload a `.bin` file in the web interface.
- If the new firmware does not run for 60 seconds, the ESP32 automatically returns to the previous version.

**Privacy / anonymous device count** – if the daily update check is enabled (setup page and settings, can be
switched off any time), the controller downloads the small file `ping-esp32-relay4.json` from the latest release once a day.
GitHub counts these downloads; the project only sees the total number – that is how the
[statistics page](https://momanufaktur.github.io/Osmonauts-Controller/) counts active devices. No device, location or usage data
is transmitted. As with every download, GitHub processes the IP address of your internet connection.

**Releases** are built automatically by GitHub Actions: change `FW_VERSION` in the sketch, then push a tag `vX.Y`.


## Pins

| Function | GPIO |
|---|---|
| Relay 1–4 (on board) – V1 supply, V2 tap, V3 standing water, V4 membrane flush | 32, 33, 25, 26 |
| Buttons 1–4 (against GND) | 16, 17, 18, 19 |
| Aqua stop sensor (analog, ADC1, 0–4095) | 34 |
| Passive buzzer | 27 |
| Status LED (on board) / BOOT button = WiFi reset | 23 / 0 |
| optional LED ring / OLED (I2C) | 13 / SDA 21, SCL 22 |

Do not use GPIO 0, 2, 12, 15 (boot) and 6–11 (flash) for buttons or sensors. GPIO 34–39 are input only without pull-ups.
