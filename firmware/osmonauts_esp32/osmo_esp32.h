#pragma once
// Osmonauts – OpenOsmosis Controller · ESP32 network module
// (c) 2026 Kai Mommsen & Elias Dörre · SPDX-License-Identifier: CC-BY-NC-SA-4.0
//
//  - WiFi setup portal (WiFiManager): hotspot "Osmonauts-XXXX" when no WiFi is known
//  - web interface http://osmonauts.local (password protected, user "admin")
//  - firmware update: upload of a .bin file or download from the latest GitHub release
//    (HTTPS, SHA-256 checked, automatic rollback if the new firmware does not run for 60 s)
//  - optional daily update check: downloads ping-<variant>.json from the latest GitHub release.
//    GitHub counts these downloads -> number of active devices, no device or usage data is sent.
//  - local statistics in flash (taps, flushes, aqua stop events ...) with start date
//  - filter change reminder with date (time from the internet), interval adjustable
//
// Safety: procedures, buttons and aqua stop run in loop() exactly as without WiFi. Web requests only
// set a pending command which loop() executes. Tapping cannot be started remotely. Firmware updates
// only in idle mode, with all valves closed; no procedure can start while an update is running.

#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <Update.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <esp_ota_ops.h>
#include <esp_random.h>
#include <mbedtls/sha256.h>
#include "osmo_webui.h"

#ifndef OSMO_CLOCK
inline bool clockValid() { return false; }   // no internet time: reminder counts operating time only
#endif

#define OSMO_USER "admin"
#define PING_URL "https://github.com/" GITHUB_REPO "/releases/latest/download/ping-" BOARD_VARIANT ".json"

// ------------------------------------------------------------------ settings & statistics (flash)
Preferences prefs;

struct NetSettings {
  String password;
  bool updateCheck;   // daily update check (= anonymous active-device count)
  bool autoUpdate;    // install updates automatically in idle mode
} netCfg;

static const char *STAT_KEYS[] = { "taps", "post", "auto", "filter", "disinf", "aqua", "stops", "boots" };
uint32_t netStats[8];
uint32_t statsSince = 0;   // date of the first start with known time (unix time)

void netLoadSettings() {
  prefs.begin("osmo", false);
  netCfg.password = prefs.getString("pw", "");
  if (netCfg.password.length() < 8) {   // first start: random password
    const char *chars = "abcdefghjkmnpqrstuvwxyz23456789";
    netCfg.password = "";
    for (int i = 0; i < 10; i++) netCfg.password += chars[esp_random() % 31];
    prefs.putString("pw", netCfg.password);
  }
  netCfg.updateCheck = prefs.getBool("upd", true);
  netCfg.autoUpdate  = prefs.getBool("auto", false);
  for (int i = 0; i < 8; i++) netStats[i] = prefs.getUInt(STAT_KEYS[i], 0);
  statsSince = prefs.getUInt("since", 0);
  netStats[7]++;
  prefs.putUInt(STAT_KEYS[7], netStats[7]);
}

void netEvent(OsmoEvent e) {
  uint8_t i = (uint8_t)e;
  if (i > 6) return;
  netStats[i]++;
  prefs.putUInt(STAT_KEYS[i], netStats[i]);
}

// ------------------------------------------------------------------ state
WiFiManager wm;
AsyncWebServer server(80);
WiFiManagerParameter pPassword("pw", "Passwort für die Weboberfläche (Benutzer: admin, mind. 8 Zeichen)", "", 32);
WiFiManagerParameter pUpdate("upd", "Täglich nach Updates suchen (GitHub zählt die Abrufe anonym)", "1", 2,
                             "type=\"checkbox\" checked style=\"width:auto\"", WFM_LABEL_AFTER);

enum NetCmd : uint8_t { CMD_NONE, CMD_STOP, CMD_FLUSH, CMD_FILTER, CMD_DISINFECT, CMD_CHECK, CMD_INSTALL, CMD_REBOOT, CMD_WIFIRESET, CMD_FILTERRESET };
volatile uint8_t pendingCmd = CMD_NONE;
volatile uint16_t pendingFilterDays = 0;

String apName;
char netMessage[96] = "";   // read by the web server task -> fixed buffer, no String reallocation
void setMessage(const String &m) { strlcpy(netMessage, m.c_str(), sizeof netMessage); }
bool webStarted = false;
bool rollbackPending = false;
unsigned long msBoot = 0, msNextCheck = 0, msResetHold = 0, msRestartAt = 0, msClockCheck = 0;

struct UpdateState {
  volatile bool busy = false;
  volatile bool available = false;
  volatile bool checked = false;
  volatile int progress = 0;
  char latest[16] = "";      // shared with the web server task -> fixed buffers
  char error[64] = "";
  String url, sha256;         // only used by the update tasks
  size_t size = 0;
} upd;

// upload via web interface
bool uploadActive = false, uploadOk = false;
String uploadError;

// ------------------------------------------------------------------ helpers
// compares "6.10" > "6.9", "6.1.1" > "6.1"
bool versionNewer(const String &a, const String &b) {
  int ia = 0, ib = 0;
  for (int k = 0; k < 4; k++) {
    long va = 0, vb = 0;
    while (ia < (int)a.length() && isDigit(a[ia])) va = va * 10 + (a[ia++] - '0');
    while (ib < (int)b.length() && isDigit(b[ib])) vb = vb * 10 + (b[ib++] - '0');
    if (va != vb) return va > vb;
    if (ia < (int)a.length()) ia++;
    if (ib < (int)b.length()) ib++;
  }
  return false;
}

// minimal JSON string / number extraction for the small, self-made manifest
String jsonValue(const String &json, const char *key) {
  String k = String("\"") + key + "\"";
  int p = json.indexOf(k);
  if (p < 0) return "";
  p = json.indexOf(':', p + k.length());
  if (p < 0) return "";
  p++;
  while (p < (int)json.length() && json[p] == ' ') p++;
  if (p < (int)json.length() && json[p] == '"') {
    int e = json.indexOf('"', p + 1);
    return e < 0 ? "" : json.substring(p + 1, e);
  }
  int e = p;
  while (e < (int)json.length() && (isDigit(json[e]) || json[e] == '.')) e++;
  return json.substring(p, e);
}

String jsonEscape(const String &s) {
  String o;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c >= 0x20) o += c;
  }
  return o;
}

String fmtDuration(unsigned long s) {
  char b[24];
  if (s >= 3600) snprintf(b, sizeof b, "%lu h %lu min", s / 3600, (s % 3600) / 60);
  else if (s >= 60) snprintf(b, sizeof b, "%lu min", s / 60);
  else snprintf(b, sizeof b, "%lu s", s);
  return b;
}

// name, duration and elapsed time of the current step (for the web interface)
unsigned long timerElapsed(const Timer &t) { return t.running ? millis() - t.start : 0; }

String stepInfo(unsigned long &total, unsigned long &elapsed) {
  total = 0; elapsed = 0;
  auto pre = [&](unsigned long dm, unsigned long ds, const char *prefix) -> String {
    if (preFlushProc.state == 1) { total = dm; elapsed = timerElapsed(preFlushProc.timer); return String(prefix) + "Membranspülung"; }
    if (preFlushProc.state == 2) { total = ds; elapsed = timerElapsed(preFlushProc.timer); return String(prefix) + "Standwasserspülung"; }
    return String(prefix) + "Vorspülung";
  };
  switch (mode) {
    case MODE_IDLE:
      if (maintenanceLock) return "Firmware-Update läuft";
      if (postFlushPending) {
        total = intervalFlushAfterTapping; elapsed = millis() - msLastWaterTapping;
        return "Bereit – Nachspülung folgt";
      }
      return "Bereit";
    case MODE_TAPPING:
      if (tapProc.state == 1) return pre(durMembraneFlushFirst, durStandWaterFlush, "Vorspülung: ");
      if (tapProc.state == 2) { total = durFilterProcessShort; elapsed = timerElapsed(filterProc.timer); return "Filtern / Zapfen"; }
      return "Wasser zapfen";
    case MODE_POST_FLUSH:
      total = durMembraneFlushSecond; elapsed = timerElapsed(postFlushProc.timer);
      return "Nachspülung Membran";
    case MODE_AUTO_FLUSH:
      if (autoFlushProc.state == 1) return pre(durMembraneFlushFirst, durStandWaterFlush, "Auto-Spülung: ");
      if (autoFlushProc.state == 2) { total = durMembraneFlushSecond; elapsed = timerElapsed(postFlushProc.timer); return "Auto-Spülung: Nachspülung"; }
      return "Auto-Spülung";
    case MODE_FILTER_CHANGE:
      if (filterChangeProc.state == 1) return pre(durMembraneFlushNewFilter, durStandWaterFlushNewFilter, "Filterwechsel: ");
      return "Filterwechsel";
    case MODE_DISINFECTION: {
      const Proc &p = disinfectionProc;
      elapsed = timerElapsed(p.timer);
      switch (p.state) {
        case DS_INJECT_1: total = intervalInjectionH2O2; return "Desinfektion: 1. H₂O₂ einspritzen";
        case DS_MOVE:     total = intervalMoveH2O2;      return "Desinfektion: H₂O₂ ins System";
        case DS_INJECT_2: total = intervalInjectionH2O2; return "Desinfektion: 2. H₂O₂ einspritzen";
        case DS_PULSE:    total = intervalMoveH2O2;      return "Desinfektion: Puls " + String(disinfectionPulse + 1) + " von " + String(disinfectionPulses);
        case DS_PAUSE:    total = intervalWaitDesinfect; return "Desinfektion: Einwirkpause " + String(disinfectionPulse + 1);
        case DS_FINAL_MEMBRANE:   total = intervalFinalFlushes; return "Desinfektion: Endspülung Membran";
        case DS_FINAL_STANDWATER: total = intervalFinalFlushes; return "Desinfektion: Endspülung Standwasser";
        case DS_FINAL_FILTER:     total = intervalFinalFlushes; return "Desinfektion: Endspülung Filtern";
        default: return "Desinfektion";
      }
    }
    case MODE_AQUASTOP:
      return "AQUASTOP – gesperrt bis Neustart";
  }
  return "";
}

// ------------------------------------------------------------------ GitHub update check / download
// HTTPS GET with redirects (GitHub release downloads redirect to a CDN)
bool httpsGet(const String &url, String &body, int &code) {
  NetworkClientSecure client;
  client.useBuiltinCACertBundle();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setTimeout(15000);
  http.setUserAgent("Osmonauts/" FW_VERSION " (" BOARD_VARIANT ")");
  if (!http.begin(client, url)) { code = -1; return false; }
  code = http.GET();
  if (code == 200) body = http.getString();
  http.end();
  return code == 200;
}

void updateCheckTask(void *) {
  String body; int code = 0;
  if (httpsGet(PING_URL, body, code)) {
    strlcpy(upd.latest, jsonValue(body, "version").c_str(), sizeof upd.latest);
    upd.url    = jsonValue(body, "bin");
    upd.sha256 = jsonValue(body, "sha256");
    upd.size   = jsonValue(body, "size").toInt();
    upd.available = strlen(upd.latest) && upd.url.startsWith("https://") && upd.sha256.length() == 64 &&
                    versionNewer(upd.latest, FW_VERSION);
    upd.error[0] = 0;
    upd.checked = true;
    Serial.printf("Update check: latest %s, installed %s\n", upd.latest, FW_VERSION);
  } else {
    strlcpy(upd.error, code == 404 ? "noch kein Release" : ("Prüfung fehlgeschlagen (" + String(code) + ")").c_str(), sizeof upd.error);
    Serial.printf("Update check failed: HTTP %d\n", code);
  }
  upd.busy = false;
  vTaskDelete(NULL);
}

void updateInstallTask(void *) {
  NetworkClientSecure client;
  client.useBuiltinCACertBundle();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setTimeout(20000);
  http.setUserAgent("Osmonauts/" FW_VERSION " (" BOARD_VARIANT ")");
  bool ok = false;
  upd.progress = 0;

  if (http.begin(client, upd.url) && http.GET() == 200) {
    int len = http.getSize();
    if (len > 0 && Update.begin(len)) {
      mbedtls_sha256_context sha;
      mbedtls_sha256_init(&sha);
      mbedtls_sha256_starts(&sha, 0);
      NetworkClient *stream = http.getStreamPtr();
      uint8_t buf[2048];
      int done = 0;
      unsigned long msLast = millis();
      while (done < len && millis() - msLast < 20000) {
        size_t avail = stream->available();
        if (!avail) { delay(5); continue; }
        int n = stream->readBytes(buf, min(avail, sizeof(buf)));
        if (n <= 0) continue;
        mbedtls_sha256_update(&sha, buf, n);
        if (Update.write(buf, n) != (size_t)n) break;
        done += n;
        upd.progress = (int)((int64_t)done * 100 / len);
        msLast = millis();
      }
      uint8_t hash[32];
      mbedtls_sha256_finish(&sha, hash);
      mbedtls_sha256_free(&sha);
      char hex[65];
      for (int i = 0; i < 32; i++) sprintf(hex + 2 * i, "%02x", hash[i]);
      if (done != len) {
        strlcpy(upd.error, "Download unvollständig", sizeof upd.error);
        Update.abort();
      } else if (!upd.sha256.equalsIgnoreCase(hex)) {
        strlcpy(upd.error, "Prüfsumme falsch – Update verworfen", sizeof upd.error);
        Update.abort();
      } else {
        ok = Update.end(true);
        if (!ok) strlcpy(upd.error, (String("Update-Fehler ") + Update.errorString()).c_str(), sizeof upd.error);
      }
    } else {
      strlcpy(upd.error, "Update konnte nicht starten", sizeof upd.error);
    }
  } else {
    strlcpy(upd.error, "Download fehlgeschlagen", sizeof upd.error);
  }
  http.end();

  if (ok) {
    Serial.printf("Update %s installed - restart\n", upd.latest);
    msRestartAt = millis() + 2000;
  } else {
    Serial.printf("Update failed: %s\n", upd.error);
    upd.available = false;     // no endless retries; the next daily check offers it again
    maintenanceLock = false;
  }
  upd.busy = false;
  vTaskDelete(NULL);
}

void startUpdateCheck() {
  if (upd.busy || WiFi.status() != WL_CONNECTED) return;
  upd.busy = true;
  xTaskCreatePinnedToCore(updateCheckTask, "osmoCheck", 10240, nullptr, 1, nullptr, 0);
}

bool startUpdateInstall() {
  if (upd.busy || !upd.available || mode != MODE_IDLE || maintenanceLock || WiFi.status() != WL_CONNECTED) return false;
  maintenanceLock = true;   // no procedure may start, valves stay closed
  upd.busy = true;
  upd.error[0] = 0;
  xTaskCreatePinnedToCore(updateInstallTask, "osmoInstall", 12288, nullptr, 1, nullptr, 0);
  return true;
}

// ------------------------------------------------------------------ web server
bool auth(AsyncWebServerRequest *r) {
  if (r->authenticate(OSMO_USER, netCfg.password.c_str())) return true;
  r->requestAuthentication("Osmonauts");
  return false;
}

String statusJson() {
  unsigned long total, elapsed;
  String step = stepInfo(total, elapsed);
  unsigned long sinceAuto = millis() - msLastAutomaticFlush;
  unsigned long nextFlush = sinceAuto >= intervalAutomaticFlush ? 0 : (intervalAutomaticFlush - sinceAuto) / 1000;
  unsigned long postIn = 0;
  if (postFlushPending) {
    unsigned long since = millis() - msLastWaterTapping;
    postIn = since >= intervalFlushAfterTapping ? 0 : (intervalFlushAfterTapping - since) / 1000;
  }
  String j;
  j.reserve(1100);
  j += "{\"version\":\"" FW_VERSION "\",\"variant\":\"" BOARD_VARIANT "\"";
  j += ",\"mode\":" + String((int)mode) + ",\"step\":\"" + jsonEscape(step) + "\"";
  j += ",\"total\":" + String(total / 1000) + ",\"elapsed\":" + String(min(elapsed, total) / 1000);
  j += ",\"relais\":" + String(relaisState);
#ifdef AQUASTOP_ENABLE
  j += ",\"aq\":{\"enabled\":true,\"value\":" + String(aquaStopValue) + ",\"threshold\":" + String(aquaStopThreshold) + ",\"count\":" + String(aquaStopCount) + "}";
#else
  j += ",\"aq\":{\"enabled\":false}";
#endif
  j += ",\"nextFlush\":" + String(nextFlush) + ",\"postPending\":" + String(postFlushPending ? "true" : "false") + ",\"postIn\":" + String(postIn);
  j += ",\"uptime\":" + String(millis() / 1000);
  j += ",\"ssid\":\"" + jsonEscape(WiFi.SSID()) + "\",\"rssi\":" + String(WiFi.RSSI()) + ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
  j += ",\"update\":{\"busy\":" + String(upd.busy ? "true" : "false") + ",\"checked\":" + String(upd.checked ? "true" : "false") +
       ",\"available\":" + String(upd.available ? "true" : "false") + ",\"latest\":\"" + jsonEscape(String(upd.latest)) +
       "\",\"progress\":" + String(upd.progress) + ",\"error\":\"" + jsonEscape(String(upd.error)) + "\"}";
  j += ",\"settings\":{\"upd\":" + String(netCfg.updateCheck ? "true" : "false") + ",\"auto\":" + String(netCfg.autoUpdate ? "true" : "false") + "}";
  j += ",\"stats\":{\"taps\":" + String(netStats[0]) + ",\"postFlushes\":" + String(netStats[1]) + ",\"autoFlushes\":" + String(netStats[2]) +
       ",\"filterChanges\":" + String(netStats[3]) + ",\"disinfections\":" + String(netStats[4]) + ",\"aquastops\":" + String(netStats[5]) +
       ",\"stops\":" + String(netStats[6]) + ",\"boots\":" + String(netStats[7]) + "}";
  time_t now = clockValid() ? time(nullptr) : 0;
  j += ",\"now\":" + String((uint32_t)now) + ",\"since\":" + String(statsSince);
  j += ",\"filter\":{\"due\":" + String(filterDue() ? "true" : "false") + ",\"daysLeft\":" + String(filterDaysLeft()) +
       ",\"minutesLeft\":" + String(filterMinutesLeft()) + ",\"interval\":" + String(filterStore.intervalDays) +
       ",\"last\":" + String(filterStore.lastChange) + ",\"clock\":" + String(now ? "true" : "false") + "}";
  j += ",\"message\":\"" + jsonEscape(String(netMessage)) + "\"}";
  return j;
}

void startWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r) {
    if (!auth(r)) return;
    r->send(200, "text/html; charset=utf-8", OSMO_PAGE);
  });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *r) {
    if (!auth(r)) return;
    r->send(200, "application/json", statusJson());
  });

  server.on("/api/cmd", HTTP_POST, [](AsyncWebServerRequest *r) {
    if (!auth(r)) return;
    String c = r->hasParam("c", true) ? r->getParam("c", true)->value() : "";
    uint8_t cmd = c == "stop" ? CMD_STOP : c == "flush" ? CMD_FLUSH : c == "filter" ? CMD_FILTER : c == "disinfect" ? CMD_DISINFECT :
                  c == "check" ? CMD_CHECK : c == "install" ? CMD_INSTALL : c == "reboot" ? CMD_REBOOT : c == "wifireset" ? CMD_WIFIRESET :
                  c == "filterreset" ? CMD_FILTERRESET : CMD_NONE;
    if (cmd == CMD_NONE) { r->send(400, "text/plain; charset=utf-8", "Unbekannter Befehl"); return; }
    pendingCmd = cmd;   // executed in loop()
    r->send(200, "text/plain; charset=utf-8", "Befehl übernommen");
  });

  server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest *r) {
    if (!auth(r)) return;
    if (r->hasParam("upd", true))  { netCfg.updateCheck = r->getParam("upd", true)->value() == "1"; prefs.putBool("upd", netCfg.updateCheck); }
    if (r->hasParam("auto", true)) { netCfg.autoUpdate  = r->getParam("auto", true)->value() == "1"; prefs.putBool("auto", netCfg.autoUpdate); }
    if (r->hasParam("filterdays", true)) {
      long d = r->getParam("filterdays", true)->value().toInt();
      if (d >= 7 && d <= 1095) pendingFilterDays = (uint16_t)d;   // applied in loop()
    }
    String msg = "Einstellungen gespeichert";
    if (r->hasParam("pw", true)) {
      String pw = r->getParam("pw", true)->value();
      if (pw.length() >= 8 && pw.length() <= 32) { netCfg.password = pw; prefs.putString("pw", pw); msg += " – neues Passwort aktiv"; }
      else if (pw.length()) msg = "Passwort muss 8 bis 32 Zeichen haben";
    }
    r->send(200, "text/plain; charset=utf-8", msg);
  });

  // firmware upload (.bin) - only in idle mode
  server.on("/api/update/upload", HTTP_POST,
    [](AsyncWebServerRequest *r) {
      if (!auth(r)) return;
      if (uploadOk) {
        r->send(200, "text/plain; charset=utf-8", "Firmware installiert – Neustart …");
        msRestartAt = millis() + 1500;
      } else {
        r->send(400, "text/plain; charset=utf-8", "Update fehlgeschlagen: " + uploadError);
      }
    },
    [](AsyncWebServerRequest *r, const String &, size_t index, uint8_t *data, size_t len, bool final) {
      if (!r->authenticate(OSMO_USER, netCfg.password.c_str())) return;
      if (index == 0) {
        uploadOk = false; uploadError = "";
        if (mode != MODE_IDLE || maintenanceLock || upd.busy) { uploadError = "nur im Leerlauf möglich"; return; }
        maintenanceLock = true;
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { uploadError = Update.errorString(); maintenanceLock = false; return; }
        uploadActive = true;
      }
      if (uploadActive && len && Update.write(data, len) != len) {
        uploadError = Update.errorString(); Update.abort(); uploadActive = false; maintenanceLock = false;
      }
      if (final && uploadActive) {
        uploadActive = false;
        uploadOk = Update.end(true);
        if (!uploadOk) { uploadError = Update.errorString(); maintenanceLock = false; }
      }
    });

  server.onNotFound([](AsyncWebServerRequest *r) { r->send(404, "text/plain", "not found"); });
  server.begin();
  MDNS.begin("osmonauts");
  MDNS.addService("http", "tcp", 80);
  webStarted = true;
  Serial.print(F("Web interface: http://osmonauts.local  or  http://"));
  Serial.println(WiFi.localIP());
}

// ------------------------------------------------------------------ WiFi setup portal
void netSaveConfig() {
  String pw = pPassword.getValue();
  if (pw.length() >= 8) { netCfg.password = pw; prefs.putString("pw", pw); }
  netCfg.updateCheck = strlen(pUpdate.getValue()) > 0;
  prefs.putBool("upd", netCfg.updateCheck);
}

// keep a new firmware only if it runs; otherwise the bootloader returns to the old one
extern "C" bool verifyRollbackLater() { return true; }

void netSetup() {
  msBoot = millis();
#ifdef STATUS_LED
  pinMode(STATUS_LED, OUTPUT);
#endif
#ifdef WIFI_RESET_PIN
  pinMode(WIFI_RESET_PIN, INPUT_PULLUP);
#endif
  netLoadSettings();

  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t state;
  if (esp_ota_get_state_partition(running, &state) == ESP_OK && state == ESP_OTA_IMG_PENDING_VERIFY) rollbackPending = true;

  WiFi.mode(WIFI_STA);
  WiFi.setHostname("osmonauts");
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.google.com");   // date for the filter reminder
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char name[24];
  snprintf(name, sizeof name, "Osmonauts-%02X%02X", mac[4], mac[5]);
  apName = name;

  pPassword.setValue(netCfg.password.c_str(), 32);
  wm.addParameter(&pPassword);
  wm.addParameter(&pUpdate);
  wm.setSaveConfigCallback(netSaveConfig);
  wm.setTitle("Osmonauts – WLAN einrichten");
  std::vector<const char *> menu = { "wifi", "info", "exit" };
  wm.setMenu(menu);
  wm.setConfigPortalBlocking(false);
  wm.setConnectTimeout(20);
  wm.setConfigPortalTimeout(wm.getWiFiIsSaved() ? 600 : 0);   // no WiFi known: portal stays open

  Serial.print(F("Web password (user admin): "));
  Serial.println(netCfg.password);
  if (wm.autoConnect(apName.c_str())) {
    Serial.println(F("WiFi connected"));
  } else {
    Serial.print(F("WiFi setup: connect to hotspot "));
    Serial.print(apName);
    Serial.println(F(" and open http://192.168.4.1"));
  }
  msNextCheck = millis() + 2UL * 60 * 1000 + (esp_random() % (30UL * 60 * 1000));   // first check 2-32 min after start
  enableLoopWDT();   // restart if loop() hangs - all valves are closed after a restart
}

// ------------------------------------------------------------------ called at the start of every loop()
void netLoop() {
  wm.process();
  if (!webStarted && WiFi.status() == WL_CONNECTED && !wm.getConfigPortalActive()) startWebServer();

  // status LED: on = procedure running, fast blink = WiFi setup portal, slow blink = aqua stop
#ifdef STATUS_LED
  bool led = mode != MODE_IDLE;
  if (wm.getConfigPortalActive()) led = (millis() / 150) % 2;
  if (mode == MODE_AQUASTOP) led = (millis() / 600) % 2;
  if (mode == MODE_IDLE && !wm.getConfigPortalActive() && filterDue()) {   // double flash: filter change due
    unsigned long t = millis() % 2000;
    led = t < 120 || (t > 240 && t < 360);
  }
  digitalWrite(STATUS_LED, led);
#endif

  // BOOT button held for 5 s: delete WiFi settings
#ifdef WIFI_RESET_PIN
  if (digitalRead(WIFI_RESET_PIN) == LOW) {
    if (!msResetHold) msResetHold = millis();
    else if (millis() - msResetHold > 5000) pendingCmd = CMD_WIFIRESET;
  } else {
    msResetHold = 0;
  }
#endif

  // new firmware runs for 60 s without problems -> keep it
  if (rollbackPending && millis() - msBoot > 60000) {
    esp_ota_mark_app_valid_cancel_rollback();
    rollbackPending = false;
    Serial.println(F("New firmware confirmed"));
  }

  // pending command from the web interface
  uint8_t cmd = pendingCmd;
  pendingCmd = CMD_NONE;
  switch (cmd) {
    case CMD_STOP:      abortCurrentMode(); setMessage("Gestoppt – alle Ventile geschlossen"); break;
    case CMD_FLUSH:     requestMode(MODE_AUTO_FLUSH);    setMessage(mode == MODE_AUTO_FLUSH ? "Spülung gestartet" : "Nicht möglich – Ablauf läuft"); break;
    case CMD_FILTER:    requestMode(MODE_FILTER_CHANGE); setMessage(mode == MODE_FILTER_CHANGE ? "Filterwechsel gestartet" : "Nicht möglich – Ablauf läuft"); break;
    case CMD_DISINFECT: requestMode(MODE_DISINFECTION);  setMessage(mode == MODE_DISINFECTION ? "Desinfektion gestartet" : "Nicht möglich – Ablauf läuft"); break;
    case CMD_CHECK:     startUpdateCheck(); setMessage("Suche nach Updates …"); break;
    case CMD_INSTALL:   setMessage(startUpdateInstall() ? "Update wird geladen …" : "Update nur im Leerlauf möglich"); break;
    case CMD_REBOOT:    msRestartAt = millis() + 500; setMessage("Neustart …"); break;
    case CMD_FILTERRESET: filterReset(); setMessage("Filterwechsel bestätigt – Countdown startet neu"); break;
    case CMD_WIFIRESET:
      stopRelais();
      wm.resetSettings();
      delay(300);
      ESP.restart();
      break;
    default: break;
  }

  // filter interval changed in the web interface
  if (pendingFilterDays) {
    filterStore.intervalDays = pendingFilterDays;
    pendingFilterDays = 0;
    filterSave();
    filterWasDue = filterDue();
    if (mode == MODE_IDLE) feedback(FB_IDLE);
  }

  // first start with known time: remember the date for the statistics
  if (!statsSince && millis() - msClockCheck > 10000) {
    msClockCheck = millis();
    if (clockValid()) { statsSince = (uint32_t)time(nullptr); prefs.putUInt("since", statsSince); }
  }

  // daily update check (= anonymous active-device count on GitHub)
  if (netCfg.updateCheck && WiFi.status() == WL_CONNECTED && (long)(millis() - msNextCheck) >= 0) {
    msNextCheck = millis() + (upd.error[0] ? 60UL : 24UL * 60) * 60 * 1000;   // retry after 1 h on errors
    startUpdateCheck();
  }
  if (netCfg.autoUpdate && upd.available && !upd.busy && mode == MODE_IDLE && !postFlushPending) startUpdateInstall();

  // restart after an update or on request - valves first
  if (msRestartAt && (long)(millis() - msRestartAt) >= 0) {
    stopRelais();
    delay(200);
    ESP.restart();
  }
}
