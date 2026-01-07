#include "web_server.h"
#include "web_pages.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <SD_MMC.h>

#include "Definitions.h"
#include "config_manager.h"
#include "wifi_manager.h"
#include "system_mode.h"
#include "storage_manager.h"   // <-- NEW

static bool webStarted = false;

// -----------------------------------------------------------------------------
// helpers
// -----------------------------------------------------------------------------

static bool isPrintableFilename(const char* s)
{
  if (!s || !*s) return false;

  for (const char* p = s; *p; ++p) {
    if (*p < 32 || *p > 126) return false;
  }
  return true;
}


static void sendJson(WebServer &s, JsonDocument &doc)
{
  String out;
  serializeJson(doc, out);
  s.send(200, "application/json", out);
}

// -----------------------------------------------------------------------------
// start server
// -----------------------------------------------------------------------------

void webserver_start(WebServer &server)
{
  if (webStarted) return;

  // ---------------------------------------------------------------------------
  // Root / SPA
  // ---------------------------------------------------------------------------
  server.on("/", HTTP_GET, [&] {
    server.send(200, "text/html", PAGE_CONFIG_APP);
  });

  server.on("/favicon.ico", HTTP_GET, [&] {
    server.send(204);
  });

  // ---------------------------------------------------------------------------
  // GET CONFIG
  // ---------------------------------------------------------------------------
  server.on("/api/config", HTTP_GET, [&] {
    StaticJsonDocument<2048> j;

    j["wifi"]["ssid"] = wifi_get_saved_ssid();

    j["system"]["cpu_freq"]     = config.cpu_freq;
    j["system"]["timezone"]     = config.timezone;
    j["system"]["timezone_DST"] = config.timezone_DST;

    j["gps"]["sample_rate"]   = config.sample_rate;
    j["gps"]["dynamic_model"] = config.dynamic_model;
    j["gps"]["cal_speed"]     = config.cal_speed;
    j["gps"]["stat_speed"]    = config.stat_speed;
    j["gps"]["start_logging_speed"] = config.start_logging_speed;

    j["power"]["shutdown_voltage"] = config.shutdown_voltage;
    j["power"]["bat_choice"]       = config.bat_choice;
    j["power"]["cal_bat"]          = config.cal_bat;

    j["logging"]["track_distance"] = config.track_distance;
    j["logging"]["archive_days"]   = config.archive_days;
    j["logging"]["file_date_time"] = config.file_date_time;

    j["logging"]["logTXT"] = config.logTXT;
    j["logging"]["logUBX"] = config.logUBX;
    j["logging"]["logSBP"] = config.logSBP;
    j["logging"]["logGPY"] = config.logGPY;
    j["logging"]["logGPX"] = config.logGPX;

    j["ui"]["field"]             = config.field;
    j["ui"]["speed_large_font"]  = config.speed_large_font;
    j["ui"]["bar_length"]        = config.bar_length;
    j["ui"]["sleep_off_screen"]  = config.sleep_off_screen;
    j["ui"]["Board_Logo"]        = config.Board_Logo;
    j["ui"]["Sail_Logo"]         = config.Sail_Logo;

    j["ui"]["Stat_screens"]      = config.Stat_screens;
    j["ui"]["Stat_screens_time"] = config.Stat_screens_time;

    j["ui"]["speed_screen"]  = config.speed_screen;
    j["ui"]["stat_screen"]   = config.stat_screen;
    j["ui"]["gpio12_screen"] = config.gpio12_screen;
    j["ui"]["Sleep_info"]    = config.Sleep_info;

    sendJson(server, j);
  });

  // ---------------------------------------------------------------------------
  // POST CONFIG
  // ---------------------------------------------------------------------------
  server.on("/api/config", HTTP_POST, [&] {

    StaticJsonDocument<2048> j;
    if (deserializeJson(j, server.arg("plain"))) {
      server.send(400, "text/plain", "Bad JSON");
      return;
    }

    if (j["wifi"]["ssid"]) {
      String ssid = j["wifi"]["ssid"].as<const char*>();
      String pass;
      if (j["wifi"]["password"])
        pass = j["wifi"]["password"].as<const char*>();
      wifi_set_credentials(ssid, pass);
    }

    if (j["system"]) {
      config.cpu_freq     = j["system"]["cpu_freq"]     | config.cpu_freq;
      config.timezone     = j["system"]["timezone"]     | config.timezone;
      config.timezone_DST = j["system"]["timezone_DST"] | config.timezone_DST;
    }

    if (j["gps"]) {
      config.sample_rate         = j["gps"]["sample_rate"] | config.sample_rate;
      config.dynamic_model       = j["gps"]["dynamic_model"] | config.dynamic_model;
      config.cal_speed           = j["gps"]["cal_speed"] | config.cal_speed;
      config.stat_speed          = j["gps"]["stat_speed"] | config.stat_speed;
      config.start_logging_speed = j["gps"]["start_logging_speed"] | config.start_logging_speed;
    }

    if (j["power"]) {
      config.shutdown_voltage = j["power"]["shutdown_voltage"] | config.shutdown_voltage;
      config.bat_choice       = j["power"]["bat_choice"] | config.bat_choice;
      config.cal_bat          = j["power"]["cal_bat"] | config.cal_bat;
    }

    if (j["logging"]) {
      config.track_distance = j["logging"]["track_distance"] | config.track_distance;
      config.archive_days   = j["logging"]["archive_days"]   | config.archive_days;
      config.file_date_time = j["logging"]["file_date_time"] | config.file_date_time;

      config.logTXT = j["logging"]["logTXT"] | config.logTXT;
      config.logUBX = j["logging"]["logUBX"] | config.logUBX;
      config.logSBP = j["logging"]["logSBP"] | config.logSBP;
      config.logGPY = j["logging"]["logGPY"] | config.logGPY;
      config.logGPX = j["logging"]["logGPX"] | config.logGPX;
    }

    if (j["ui"]) {
      config.field            = j["ui"]["field"] | config.field;
      config.speed_large_font = j["ui"]["speed_large_font"] | config.speed_large_font;
      config.bar_length       = j["ui"]["bar_length"] | config.bar_length;
      config.sleep_off_screen = j["ui"]["sleep_off_screen"] | config.sleep_off_screen;
      config.Board_Logo       = j["ui"]["Board_Logo"] | config.Board_Logo;
      config.Sail_Logo        = j["ui"]["Sail_Logo"] | config.Sail_Logo;

      config.Stat_screens      = j["ui"]["Stat_screens"] | config.Stat_screens;
      config.Stat_screens_time = j["ui"]["Stat_screens_time"] | config.Stat_screens_time;

      if (j["ui"]["speed_screen"])
        strlcpy(config.speed_screen, j["ui"]["speed_screen"], sizeof(config.speed_screen));

      if (j["ui"]["stat_screen"])
        strlcpy(config.stat_screen, j["ui"]["stat_screen"], sizeof(config.stat_screen));

      if (j["ui"]["gpio12_screen"])
        strlcpy(config.gpio12_screen, j["ui"]["gpio12_screen"], sizeof(config.gpio12_screen));

      if (j["ui"]["Sleep_info"])
        strlcpy(config.Sleep_info, j["ui"]["Sleep_info"], sizeof(config.Sleep_info));
    }

    saveConfig();
    server.send(200, "text/plain", "OK");
  });

  // ---------------------------------------------------------------------------
  // NET STATUS
  // ---------------------------------------------------------------------------
  server.on("/api/netstatus", HTTP_GET, [&] {
    StaticJsonDocument<256> j;
    j["sta"] = wifi_sta_connected();
    if (wifi_sta_connected()) {
      j["ssid"] = wifi_sta_ssid();
      j["ip"]   = wifi_sta_ip();
    }
    sendJson(server, j);
  });

  // ---------------------------------------------------------------------------
  // FILE LIST
  // ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// FILE LIST  (SD card only, hardened)
// ---------------------------------------------------------------------------
server.on("/api/files", HTTP_GET, [&] {

  StaticJsonDocument<2048> j;

  // SD not available
  if (!sdOK) {
    j["ok"] = false;
    sendJson(server, j);
    return;
  }

  j["ok"]      = true;
  j["free_kb"] = storageFreeKBytes();
  JsonArray arr = j.createNestedArray("files");

  fs::FS& fs = SD_MMC;
  File root = fs.open("/");
  if (!root || !root.isDirectory()) {
    sendJson(server, j);
    return;
  }

  while (true) {
    File f = root.openNextFile();
    if (!f) break;

    // -----------------------------------------------------------------------
    // HARD FILTERS
    // -----------------------------------------------------------------------

    // Never list directories
    if (f.isDirectory()) {
      f.close();
      continue;
    }

    const char* name = f.name();
    if (!name) {
      f.close();
      continue;
    }

    // Reject historical JSON / API garbage
    if (name[0] == '"' ||
        strstr(name, "ok") == name ||
        strstr(name, "free_kb") ||
        strstr(name, "{") ||
        strstr(name, "}")) {
      f.close();
      continue;
    }

    // Ignore zero-length junk
    if (f.size() == 0) {
      f.close();
      continue;
    }

    // Only allow real GPS log files
    const char* ext = strrchr(name, '.');
    if (!ext ||
        (strcmp(ext, ".sbp") &&
         strcmp(ext, ".ubx") &&
         strcmp(ext, ".gpx"))) {
      f.close();
      continue;
    }

    // -----------------------------------------------------------------------
    // VALID FILE
    // -----------------------------------------------------------------------
    JsonObject o = arr.createNestedObject();
    o["name"] = name;
    o["size"] = f.size();

    f.close();
  }

  root.close();
  sendJson(server, j);
});



  // ---------------------------------------------------------------------------
  // FILE DOWNLOAD
  // ---------------------------------------------------------------------------
  server.on("/api/file", HTTP_GET, [&] {
    if (!sdOK || !server.hasArg("name")) {
      server.send(404);
      return;
    }

    String path = "/" + server.arg("name");
    File f = SD_MMC.open(path, FILE_READ);
    if (!f) {
      server.send(404);
      return;
    }

    server.streamFile(f, "application/octet-stream");
    f.close();
  });

  // ---------------------------------------------------------------------------
  // FILE DELETE
  // ---------------------------------------------------------------------------
  server.on("/api/file", HTTP_DELETE, [&] {
    if (!sdOK) {
      server.send(200, "application/json", "{\"ok\":false}");
      return;
    }

    StaticJsonDocument<256> j;
    deserializeJson(j, server.arg("plain"));
    String path = "/" + String((const char*)j["name"]);

    bool ok = SD_MMC.remove(path);
    server.send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
  });

  // ---------------------------------------------------------------------------
  // WIFI CONNECT
  // ---------------------------------------------------------------------------
  server.on("/api/wifi/connect", HTTP_POST, [&] {
    server.send(200, "text/plain", "OK");
    delay(50);
    wifi_start_sta();
  });

  server.begin();
  webStarted = true;
  LOG_WIFI("Web", "started");
}

// -----------------------------------------------------------------------------
// stop
// -----------------------------------------------------------------------------

void webserver_stop()
{
  webStarted = false;
}

// -----------------------------------------------------------------------------
// STA helpers
// -----------------------------------------------------------------------------

bool wifi_sta_connected()
{
  return WiFi.status() == WL_CONNECTED;
}

String wifi_sta_ssid()
{
  return wifi_sta_connected() ? WiFi.SSID() : "";
}

String wifi_sta_ip()
{
  return wifi_sta_connected() ? WiFi.localIP().toString() : "";
}
