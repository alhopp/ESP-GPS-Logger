// ============================================================================
// web_server.cpp
// HTTP server wiring for ESP32 GPS Logger (STA-only)
//
// - Serves SPA UI from LittleFS
// - Exposes JSON APIs (config, system, net status)
// - Serves SD logs + offline tiles
// - Requires Wi-Fi STA already connected
// ============================================================================

#include "web/web_server.h"
#include "web/web_files.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <SD_MMC.h>

#include "config_manager.h"
#include "system_info.h"
#include "Definitions.h"

#include <esp_system.h>

// -----------------------------------------------------------------------------
// Server lifecycle guard
// -----------------------------------------------------------------------------
static WebServer server(80);   
static bool webStarted = false;

// -----------------------------------------------------------------------------
// JSON send helper
// -----------------------------------------------------------------------------
static void sendJson(WebServer &s, JsonDocument &doc)
{
  String out;
  serializeJson(doc, out);
  s.send(200, "application/json", out);
}

// -----------------------------------------------------------------------------
// Start HTTP server (STA-only)
// -----------------------------------------------------------------------------
void webserver_start()
{
  if (webStarted) return;

  // ---------------------------------------------------------------------------
  // GET /api/config
  // ---------------------------------------------------------------------------
  server.on("/api/config", HTTP_GET, [&] {
    DynamicJsonDocument j(3072);

    // ---- Wi-Fi (read-only) ----
    JsonObject wifi = j.createNestedObject("wifi");
    wifi["connected"] = WiFi.status() == WL_CONNECTED;
    wifi["ssid"]      = WiFi.SSID();
    wifi["ip"]        = WiFi.localIP().toString();

    // ---- System info ----
    JsonObject system = j.createNestedObject("system");
    system["gnss_module"]      = systemInfo.gnss_module;
    system["gnss_mode"]        = systemInfo.gnss_mode;
    system["dynamic_model"]    = systemInfo.dynamic_model;
    system["sample_rate"]      = systemInfo.sample_rate;
    system["storage_mb"]       = systemInfo.storage_mb;
    system["software_version"] = systemInfo.software_version;
    system["display"]          = systemInfo.display;
    system["cpu_freq"]         = systemInfo.cpu_freq;
    system["speed_units"]      = systemInfo.speed_units;
    system["cal_speed"]        = systemInfo.cal_speed;

    // ---- User config ----
    JsonObject configJ = j.createNestedObject("config");
    configJ["timezone"]     = config.timezone;
    configJ["timezone_DST"] = config.timezone_DST;

    JsonObject power = j.createNestedObject("power");
    power["cal_bat"] = config.cal_bat;

    JsonObject logging = j.createNestedObject("logging");
    logging["logTXT"] = config.track_distance;
    logging["logUBX"] = config.logUBX;
    logging["logSBP"] = config.logSBP;

    JsonObject ui = j.createNestedObject("ui");
    ui["bar_length"] = config.bar_length;
    ui["Sleep_info"] = config.Sleep_info;

    JsonObject stats = j.createNestedObject("stats");
    stats["s2"]       = config.stat_2s;
    stats["s10"]      = config.stat_10s;
    stats["alpha"]    = config.stat_alpha;
    stats["nm"]       = config.stat_nm;
    stats["h1"]       = config.stat_1h;
    stats["distance"] = config.stat_distance;

    sendJson(server, j);
  });

  // ---------------------------------------------------------------------------
  // POST /api/config
  // ---------------------------------------------------------------------------
  server.on("/api/config", HTTP_POST, [&] {
    DynamicJsonDocument j(2048);
    if (deserializeJson(j, server.arg("plain"))) {
      server.send(400, "text/plain", "Bad JSON");
      return;
    }

    if (j["logging"]) {
      if (j["logging"]["logTXT"] != nullptr) config.track_distance = j["logging"]["logTXT"];
      if (j["logging"]["logUBX"] != nullptr) config.logUBX = j["logging"]["logUBX"];
      if (j["logging"]["logSBP"] != nullptr) config.logSBP = j["logging"]["logSBP"];
    }

    if (j["stats"]) {
      if (j["stats"]["s2"] != nullptr)       config.stat_2s = j["stats"]["s2"];
      if (j["stats"]["s10"] != nullptr)      config.stat_10s = j["stats"]["s10"];
      if (j["stats"]["alpha"] != nullptr)    config.stat_alpha = j["stats"]["alpha"];
      if (j["stats"]["nm"] != nullptr)       config.stat_nm = j["stats"]["nm"];
      if (j["stats"]["h1"] != nullptr)       config.stat_1h = j["stats"]["h1"];
      if (j["stats"]["distance"] != nullptr) config.stat_distance = j["stats"]["distance"];
    }

    if (j["ui"]["Sleep_info"] != nullptr)
      strlcpy(config.Sleep_info,
              j["ui"]["Sleep_info"].as<const char*>(),
              sizeof(config.Sleep_info));

    saveConfig();
    server.send(200, "text/plain", "OK");
  });

  // ---------------------------------------------------------------------------
  // GET /api/netstatus
  // ---------------------------------------------------------------------------
  server.on("/api/netstatus", HTTP_GET, [&] {
    StaticJsonDocument<256> j;
    j["connected"] = WiFi.status() == WL_CONNECTED;
    j["ssid"]      = WiFi.SSID();
    j["ip"]        = WiFi.localIP().toString();
    sendJson(server, j);
  });

  // ---------------------------------------------------------------------------
  // SD / log file APIs
  // ---------------------------------------------------------------------------
  registerFileEndpoints(server);

  // ---------------------------------------------------------------------------
  // Root UI
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
  // Noise suppression
  // ---------------------------------------------------------------------------
  server.on("/favicon.ico", HTTP_GET, [&]{ server.send(204); });
  server.on("/apple-touch-icon.png", HTTP_GET, [&]{ server.send(204); });
  server.on("/apple-touch-icon-precomposed.png", HTTP_GET, [&]{ server.send(204); });

  // ---------------------------------------------------------------------------
  // Static assets (LittleFS)
  // ---------------------------------------------------------------------------
  server.serveStatic("/", LittleFS, "/");

  // ---------------------------------------------------------------------------
  // Not found
  // ---------------------------------------------------------------------------
  server.onNotFound([&] {
    server.send(204);
  });

  server.begin();
  webStarted = true;
  LOG_WIFI("Web", "started (STA-only)");
}

// -----------------------------------------------------------------------------
// Stop server
// -----------------------------------------------------------------------------
void webserver_stop()
{
  webStarted = false;
}

void webserver_loop()
{
  server.handleClient();
}

