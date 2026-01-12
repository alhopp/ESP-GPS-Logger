// ============================================================================
// web_server.cpp
//
// Main HTTP server wiring for the ESP32 GPS Logger.
//
// Responsibilities:
// - Serve the single-page configuration UI (SPA)
// - Expose REST-style JSON APIs for:
//     • System configuration (GET / POST)
//     • Network status
//     • Wi-Fi connection control
// - Delegate SD / file APIs to web_files.cpp
//
// Design notes:
// - Runs only in CONFIG / SoftAP mode
// - Server startup is guarded to prevent double-initialisation
// - File APIs are registered externally via registerFileEndpoints()
//
// ============================================================================

#include "web/web_server.h"
#include "web/web_pages.h"
#include "web/web_files.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "Definitions.h"
#include "config_manager.h"
#include "web/wifi_manager.h"
#include "system_info.h"

// -----------------------------------------------------------------------------
// Server lifecycle state
// -----------------------------------------------------------------------------

// Guards against starting the web server more than once
static bool webStarted = false;

// -----------------------------------------------------------------------------
// JSON helpers
// -----------------------------------------------------------------------------

// Serialize a JsonDocument and send it as an HTTP JSON response.
// Centralised here so handlers stay small and consistent.
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
  // Prevent double-start
  if (webStarted) return;


  // Serve the main configuration UI
  server.on("/", HTTP_GET, [&] {server.send(200, "text/html", PAGE_CONFIG_APP); });

  // iOS Safari noise suppression
  server.on("/apple-touch-icon.png", HTTP_GET, [&] { server.send(204); });
  server.on("/apple-touch-icon-precomposed.png", HTTP_GET, [&] { server.send(204); });
  server.on("/favicon.ico", HTTP_GET, [&] { server.send(204); });

  // Common browser probes (harmless but noisy)
  server.on("/manifest.json", HTTP_GET, [&] { server.send(204); });
  server.on("/robots.txt", HTTP_GET, [&] { server.send(204); });
  server.on("/service-worker.js", HTTP_GET, [&] { server.send(204); });


  // ---------------------------------------------------------------------------
  // GET CONFIG
  // Returns the current system configuration as JSON
  // ---------------------------------------------------------------------------
  server.on("/api/config", HTTP_GET, [&] {
    DynamicJsonDocument j(2048);

    // -------------------------------------------------------------------------
    // Wi-Fi
    // -------------------------------------------------------------------------
    j["wifi"]["ssid"] = wifi_get_saved_ssid();

    // -------------------------------------------------------------------------
    // System (read-only / firmware-defined)
    // -------------------------------------------------------------------------
    j["system"]["cpu_freq"]         = config.cpu_freq;
    j["system"]["timezone"]         = config.timezone;
    j["system"]["timezone_DST"]     = config.timezone_DST;
    j["system"]["gnss_module"]      = systemInfo.gnss_module;
    j["system"]["storage_mb"]       = systemInfo.storage_mb;
    j["system"]["software_version"] = systemInfo.software_version;
    j["system"]["display"]          = systemInfo.display;

    // -------------------------------------------------------------------------
    // GPS
    // -------------------------------------------------------------------------
    j["gps"]["speed_units"]         = systemInfo.speed_units;
    j["gps"]["sample_rate"]         = systemInfo.sample_rate;
    j["gps"]["gnss"]                = systemInfo.gnss_mode;
    j["gps"]["dynamic_model"]       = systemInfo.dynamic_model;
    j["gps"]["cal_speed"]           = config.cal_speed;
    j["gps"]["stat_speed"]          = config.stat_speed;
    j["gps"]["start_logging_speed"] = config.start_logging_speed;

    // -------------------------------------------------------------------------
    // Power
    // -------------------------------------------------------------------------
    j["power"]["bat_choice"] = config.bat_choice;
    j["power"]["cal_bat"]    = config.cal_bat;

    // -------------------------------------------------------------------------
    // Logging
    // -------------------------------------------------------------------------
    j["logging"]["track_distance"] = config.track_distance;
    j["logging"]["archive_days"]   = config.archive_days;
    j["logging"]["file_date_time"] = config.file_date_time;
    j["logging"]["logTXT"]         = config.logTXT;
    j["logging"]["logUBX"]         = config.logUBX;
    j["logging"]["logSBP"]         = config.logSBP;
    j["logging"]["logGPY"]         = config.logGPY;
    j["logging"]["logGPX"]         = config.logGPX;

    // -------------------------------------------------------------------------
    // UI
    // -------------------------------------------------------------------------
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

  // ---------------------------------------------------------------------------
  // POST CONFIG
  //
  // Updates system configuration from JSON payload
  // ---------------------------------------------------------------------------
  server.on("/api/config", HTTP_POST, [&] {

    DynamicJsonDocument j(2048);
    if (deserializeJson(j, server.arg("plain"))) {
      server.send(400, "text/plain", "Bad JSON");
      return;
    }

    // Wi-Fi credentials
    if (j["wifi"]["ssid"]) {
      String ssid = j["wifi"]["ssid"].as<const char*>();
      String pass;
      if (j["wifi"]["password"])
        pass = j["wifi"]["password"].as<const char*>();
      wifi_set_credentials(ssid, pass);
    }

    // System
    if (j["system"].containsKey("cpu_freq"))     config.cpu_freq     = j["system"]["cpu_freq"];
    if (j["system"].containsKey("timezone"))     config.timezone     = j["system"]["timezone"];
    if (j["system"].containsKey("timezone_DST")) config.timezone_DST = j["system"]["timezone_DST"];

    // GPS
    if (j["gps"]) {
      if (j["gps"].containsKey("sample_rate"))         config.sample_rate         = j["gps"]["sample_rate"];
      if (j["gps"].containsKey("dynamic_model"))       config.dynamic_model       = j["gps"]["dynamic_model"];
      if (j["gps"].containsKey("cal_speed"))           config.cal_speed           = j["gps"]["cal_speed"];
      if (j["gps"].containsKey("stat_speed"))          config.stat_speed          = j["gps"]["stat_speed"];
      if (j["gps"].containsKey("start_logging_speed")) config.start_logging_speed = j["gps"]["start_logging_speed"];
    }

    // Power
    if (j["power"]) {
      if (j["power"].containsKey("shutdown_voltage")) config.shutdown_voltage = j["power"]["shutdown_voltage"];
      if (j["power"].containsKey("bat_choice"))       config.bat_choice       = j["power"]["bat_choice"];
      if (j["power"].containsKey("cal_bat"))          config.cal_bat          = j["power"]["cal_bat"];
    }

    // Logging
    if (j["logging"]) {
      if (j["logging"].containsKey("track_distance")) config.track_distance = j["logging"]["track_distance"];
      if (j["logging"].containsKey("archive_days"))   config.archive_days   = j["logging"]["archive_days"];
      if (j["logging"].containsKey("file_date_time")) config.file_date_time = j["logging"]["file_date_time"];
      if (j["logging"].containsKey("logTXT"))         config.logTXT         = j["logging"]["logTXT"];
      if (j["logging"].containsKey("logUBX"))         config.logUBX         = j["logging"]["logUBX"];
      if (j["logging"].containsKey("logSBP"))         config.logSBP         = j["logging"]["logSBP"];
      if (j["logging"].containsKey("logGPY"))         config.logGPY         = j["logging"]["logGPY"];
      if (j["logging"].containsKey("logGPX"))         config.logGPX         = j["logging"]["logGPX"];
    }

    // UI
    if (j["ui"]) {
      if (j["ui"].containsKey("field"))            config.field            = j["ui"]["field"];
      if (j["ui"].containsKey("speed_large_font")) config.speed_large_font = j["ui"]["speed_large_font"];
      if (j["ui"].containsKey("bar_length"))       config.bar_length       = j["ui"]["bar_length"];
      if (j["ui"].containsKey("sleep_off_screen")) config.sleep_off_screen = j["ui"]["sleep_off_screen"];
      if (j["ui"].containsKey("Board_Logo"))       config.Board_Logo       = j["ui"]["Board_Logo"];
      if (j["ui"].containsKey("Sail_Logo"))        config.Sail_Logo        = j["ui"]["Sail_Logo"];
      if (j["ui"].containsKey("Stat_screens"))     config.Stat_screens     = j["ui"]["Stat_screens"];
      if (j["ui"].containsKey("Stat_screens_time"))config.Stat_screens_time= j["ui"]["Stat_screens_time"];

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
  // WIFI CONNECT
  // ---------------------------------------------------------------------------
  server.on("/api/wifi/connect", HTTP_POST, [&] {
    server.send(200, "text/plain", "OK");
    delay(50);
    wifi_start_sta();
  });

  // ---------------------------------------------------------------------------
  // FILE APIs (delegated)
  // ---------------------------------------------------------------------------
  registerFileEndpoints(server);

  // ---------------------------------------------------------------------------
  // Catch-all: silence unknown browser requests
  // ---------------------------------------------------------------------------
  server.onNotFound([&]() {
    server.send(204);
  });

  // ---------------------------------------------------------------------------
  // Start server
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
