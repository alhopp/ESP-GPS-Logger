#include "web_server.h"
#include "web_pages.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "Definitions.h"
#include "system_mode.h"
#include "config_manager.h"
#include "wifi_manager.h"

static bool webStarted = false;

// ============================================================================
// Web server start
// ============================================================================

void webserver_start(WebServer &server)
{
  if (webStarted) return;

  // ------------------------------------------------------------
  // Root: main config app
  // ------------------------------------------------------------
  server.on("/", HTTP_GET, [&server]() {
    server.send(200, "text/html", PAGE_CONFIG_APP);
  });

  // ------------------------------------------------------------
  // API: GET current configuration (READ ONLY)
  // ------------------------------------------------------------
  server.on("/api/config", HTTP_GET, [&server]() {

    StaticJsonDocument<1024> doc;

    // -------------------------
    // Wi-Fi
    // -------------------------
    doc["wifi"]["ssid"] = config.ssid;

    // -------------------------
    // System
    // -------------------------
    doc["system"]["cpu_freq"]     = config.cpu_freq;
    doc["system"]["timezone"]     = config.timezone;
    doc["system"]["timezone_dst"] = config.timezone_DST;

    // -------------------------
    // GPS
    // -------------------------
    doc["gps"]["sample_rate"]   = config.sample_rate;
    doc["gps"]["gnss"]          = config.gnss;
    doc["gps"]["dynamic_model"] = config.dynamic_model;
    doc["gps"]["cal_speed"]     = config.cal_speed;

    // -------------------------
    // Power
    // -------------------------
    doc["power"]["shutdown_voltage"] = config.shutdown_voltage;
    doc["power"]["bat_choice"]       = config.bat_choice;

    // -------------------------
    // Meta / diagnostics
    // -------------------------
    doc["meta"]["ublox_type"]   = config.ublox_type;
    doc["meta"]["m10_high_nav"] = config.M10_high_nav;

    String json;
    serializeJson(doc, json);

    server.send(200, "application/json", json);
  });

  // ------------------------------------------------------------
  // Save Wi-Fi (legacy HTML POST)
  // ------------------------------------------------------------
  server.on("/save_wifi", HTTP_POST, [&server]() {

    wifi_set_credentials(
      server.arg("ssid"),
      server.arg("pass")
    );

    server.send(200, "text/html",
      "<h3>Wi-Fi saved</h3><p>Switching to HOME mode.</p>"
    );

    delay(300);
    setMode(MODE_HOME);
  });

  // ------------------------------------------------------------
  // Save config (HTML form – mapping later)
  // ------------------------------------------------------------
  server.on("/save_config", HTTP_POST, [&server]() {

    // TODO:
    //  - Map fields to config
    //  - Validate
    //  - saveConfig()

    saveConfig();

    server.send(200, "text/html",
      "<h3>Config saved</h3><p>Changes applied.</p>"
    );
  });

  // ------------------------------------------------------------
  // API: reboot
  // ------------------------------------------------------------
  server.on("/api/reboot", HTTP_POST, [&server]() {
    server.send(200, "text/plain", "Rebooting");
    delay(200);
    ESP.restart();
  });

  // ------------------------------------------------------------
  // Fallback
  // ------------------------------------------------------------
  server.onNotFound([&server]() {
    server.send(200, "text/html", PAGE_CONFIG_APP);
  });

  server.begin();
  webStarted = true;

  LOG_WIFI("Web", "started");
}

// ============================================================================
// Web server stop
// ============================================================================

void webserver_stop()
{
  webStarted = false;
}
