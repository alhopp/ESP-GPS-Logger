// ============================================================================
// web_server.cpp
//
// Main HTTP server wiring for the ESP32 GPS Logger.
//
// Responsibilities:
// - Serve the single-page configuration UI (SPA)
// - Expose REST-style JSON APIs
// - Delegate SD / file APIs to web_files.cpp
//
// Design notes:
// - Runs only in CONFIG / SoftAP mode
// - Server startup is guarded to prevent double-initialisation
// ============================================================================

#include "web/web_server.h"
#include "web/web_files.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "Definitions.h"
#include "config_manager.h"
#include "web/wifi_manager.h"
#include "system_info.h"

#include <LittleFS.h>

// -----------------------------------------------------------------------------
// Server lifecycle state
// -----------------------------------------------------------------------------

static bool webStarted = false;

// -----------------------------------------------------------------------------
// JSON helper
// -----------------------------------------------------------------------------

static void sendJson(WebServer &s, JsonDocument &doc)
{
  String out;
  serializeJson(doc, out);
  s.send(200, "application/json", out);
}

// -----------------------------------------------------------------------------
// Web server start
// -----------------------------------------------------------------------------

void webserver_start(WebServer &server)
{
  if (webStarted) return;

  // ---------------------------------------------------------------------------
  // API ROUTES (REGISTER FIRST)
  // ---------------------------------------------------------------------------

  server.on("/api/config", HTTP_GET, [&] {
    DynamicJsonDocument j(2048);

    j["wifi"]["ssid"]               = wifi_get_saved_ssid();

    j["system"]["cpu_freq"]         = config.cpu_freq;
    j["system"]["timezone"]         = config.timezone;
    j["system"]["timezone_DST"]     = config.timezone_DST;
    j["system"]["gnss_module"]      = systemInfo.gnss_module;
    j["system"]["storage_mb"]       = systemInfo.storage_mb;
    j["system"]["software_version"] = systemInfo.software_version;
    j["system"]["display"]          = systemInfo.display;

    j["gps"]["speed_units"]         = systemInfo.speed_units;
    j["gps"]["sample_rate"]         = systemInfo.sample_rate;
    j["gps"]["gnss"]                = systemInfo.gnss_mode;
    j["gps"]["dynamic_model"]       = systemInfo.dynamic_model;
    j["gps"]["cal_speed"]           = config.cal_speed;
    j["gps"]["stat_speed"]          = config.stat_speed;
    j["gps"]["start_logging_speed"] = config.start_logging_speed;

    j["power"]["bat_choice"] = config.bat_choice;
    j["power"]["cal_bat"]    = config.cal_bat;

    j["logging"]["track_distance"] = config.track_distance;
    j["logging"]["archive_days"]   = config.archive_days;
    j["logging"]["file_date_time"] = config.file_date_time;
    j["logging"]["logTXT"]         = config.logTXT;
    j["logging"]["logUBX"]         = config.logUBX;
    j["logging"]["logSBP"]         = config.logSBP;

    j["ui"]["field"]             = config.field;
    j["ui"]["speed_large_font"]  = config.speed_large_font;
    j["ui"]["bar_length"]        = config.bar_length;
    j["ui"]["sleep_off_screen"]  = config.sleep_off_screen;
    j["ui"]["Board_Logo"]        = config.Board_Logo;
    j["ui"]["Sail_Logo"]         = config.Sail_Logo;
    j["ui"]["Stat_screens"]      = config.Stat_screens;
    j["ui"]["Stat_screens_time"] = config.Stat_screens_time;
    j["ui"]["speed_screen"]      = config.speed_screen;
    j["ui"]["stat_screen"]       = config.stat_screen;
    j["ui"]["gpio12_screen"]     = config.gpio12_screen;
    j["ui"]["Sleep_info"]        = config.Sleep_info;

    sendJson(server, j);
  });

  server.on("/api/config", HTTP_POST, [&] {
    DynamicJsonDocument j(2048);
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

  // ---------------- Logging ----------------
  if (j["logging"]) 
  {
    if (j["logging"]["logUBX"] != nullptr)config.logUBX = j["logging"]["logUBX"];
    if (j["logging"]["logSBP"] != nullptr)config.logSBP = j["logging"]["logSBP"];
  }


    saveConfig();
    server.send(200, "text/plain", "OK");
  });

  server.on("/api/netstatus", HTTP_GET, [&] {
    StaticJsonDocument<256> j;
    j["sta"] = wifi_sta_connected();
    if (wifi_sta_connected()) {
      j["ssid"] = wifi_sta_ssid();
      j["ip"]   = wifi_sta_ip();
    }
    sendJson(server, j);
  });

  server.on("/api/wifi/connect", HTTP_POST, [&] {
    server.send(200, "text/plain", "OK");
    delay(50);
    wifi_start_sta();
  });

  // File APIs (SD, logs, etc.)
  registerFileEndpoints(server);

  // ---------------------------------------------------------------------------
  // ROOT PAGE (EXPLICIT)
  // ---------------------------------------------------------------------------

  server.on("/", HTTP_GET, [&] {
    File f = LittleFS.open("/index.html", "r");
    if (!f) {
      server.send(404, "text/plain", "index.html missing");
      return;
    }
    server.streamFile(f, "text/html");
    f.close();
  });

  // ---------------------------------------------------------------------------
  // STATIC FILES (CSS / JS / IMAGES)
  // ---------------------------------------------------------------------------

  server.serveStatic("/", LittleFS, "/");

  // ---------------------------------------------------------------------------
  // BROWSER NOISE (OPTIONAL, CLEAN LOGS)
  // ---------------------------------------------------------------------------

  server.on("/favicon.ico", HTTP_GET, []{});
  server.on("/apple-touch-icon.png", HTTP_GET, []{});
  server.on("/apple-touch-icon-precomposed.png", HTTP_GET, []{});
  server.on("/manifest.json", HTTP_GET, []{});
  server.on("/robots.txt", HTTP_GET, []{});
  server.on("/service-worker.js", HTTP_GET, []{});

  // ---------------------------------------------------------------------------
  // FALLBACK
  // ---------------------------------------------------------------------------

  server.onNotFound([&] {server.send(204);});


  // ---------------------------------------------------------------------------
  // START SERVER (LAST)
  // ---------------------------------------------------------------------------

  server.begin();
  webStarted = true;
  LOG_WIFI("Web", "started");
}

// -----------------------------------------------------------------------------
// Stop server
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
