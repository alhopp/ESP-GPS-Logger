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

static bool webStarted = false;

// -----------------------------------------------------------------------------
// helpers
// -----------------------------------------------------------------------------

static void sendJson(WebServer &s, JsonDocument &doc)
{
  String out;
  serializeJson(doc, out);
  s.send(200, "application/json", out);
}

static bool isUserFile(const String& name)
{
  return !(name.startsWith("config") ||
           name.startsWith("emmc")  ||
           name.endsWith(".cfg"));
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

  // ---------------------------------------------------------------------------
  // Captive portal endpoints
  // ---------------------------------------------------------------------------
  server.on("/hotspot-detect.html", HTTP_GET, [&] {
    server.send(200, "text/html", PAGE_CONFIG_APP);
  });

  server.on("/library/test/success.html", HTTP_GET, [&] {
    server.send(200, "text/html", PAGE_CONFIG_APP);
  });

  server.on("/generate_204", HTTP_GET, [&] {
    server.send(200, "text/html", PAGE_CONFIG_APP);
  });

  server.on("/ncsi.txt", HTTP_GET, [&] {
    server.send(200, "text/html", PAGE_CONFIG_APP);
  });

  server.on("/connecttest.txt", HTTP_GET, [&] {
    server.send(200, "text/html", PAGE_CONFIG_APP);
  });

  server.on("/favicon.ico", HTTP_GET, [&] {
    server.send(204);
  });

  // ---------------------------------------------------------------------------
  // GET config (UI autofill)
  // ---------------------------------------------------------------------------
  server.on("/api/config", HTTP_GET, [&] {
    StaticJsonDocument<1024> j;

    // Wi-Fi
    JsonObject wifi = j.createNestedObject("wifi");
    wifi["ssid"]     = wifi_get_saved_ssid();
    wifi["password"] = wifi_get_saved_pass();

    // System
    j["system"]["cpu_freq"]     = config.cpu_freq;
    j["system"]["timezone"]     = config.timezone;
    j["system"]["timezone_dst"] = config.timezone_DST;

    // GPS
    j["gps"]["sample_rate"]   = config.sample_rate;
    j["gps"]["gnss"]          = config.gnss;
    j["gps"]["dynamic_model"] = config.dynamic_model;
    j["gps"]["cal_speed"]     = config.cal_speed;

    // Power
    j["power"]["shutdown_voltage"] = config.shutdown_voltage;
    j["power"]["bat_choice"]       = config.bat_choice;

    sendJson(server, j);
  });

  // ---------------------------------------------------------------------------
  // POST config
  // ---------------------------------------------------------------------------
  server.on("/api/config", HTTP_POST, [&] {

    LOG_SYS("WEB", "POST /api/config HIT");

    String raw = server.arg("plain");
    LOG_SYS("WEB", "RAW JSON: %s", raw.c_str());

    StaticJsonDocument<1024> j;
    if (deserializeJson(j, raw)) {
      server.send(400, "text/plain", "Bad JSON");
      return;
    }

    // Wi-Fi
    if (j["wifi"]["ssid"]) {
      String ssid = j["wifi"]["ssid"].as<const char*>();
      String pass;

      if (j["wifi"]["password"] &&
          strlen(j["wifi"]["password"]) > 0) {
        pass = j["wifi"]["password"].as<const char*>();
      }

      wifi_set_credentials(ssid, pass);
    }

    // System
    if (j["system"]) {
      config.cpu_freq     = j["system"]["cpu_freq"]     | config.cpu_freq;
      config.timezone     = j["system"]["timezone"]     | config.timezone;
      config.timezone_DST = j["system"]["timezone_dst"] | config.timezone_DST;
    }

    // GPS
    if (j["gps"]) {
      config.sample_rate   = j["gps"]["sample_rate"]   | config.sample_rate;
      config.gnss          = j["gps"]["gnss"]          | config.gnss;
      config.dynamic_model = j["gps"]["dynamic_model"] | config.dynamic_model;
      config.cal_speed     = j["gps"]["cal_speed"]     | config.cal_speed;
    }

    // Power
    if (j["power"]) {
      config.shutdown_voltage = j["power"]["shutdown_voltage"] | config.shutdown_voltage;
      config.bat_choice       = j["power"]["bat_choice"]       | config.bat_choice;
    }

    saveConfig();
    server.send(200, "text/plain", "OK");

    delay(300);
    wifi_start_sta();
  });

  // ---------------------------------------------------------------------------
  // STA network status (Leaflet / internet detection)
  // ---------------------------------------------------------------------------
  server.on("/api/netstatus", HTTP_GET, [&] {

    bool sta = wifi_sta_connected();

    String json = "{";
    json += "\"sta\":";
    json += sta ? "true" : "false";

    if (sta) {
      json += ",\"ssid\":\"";
      json += wifi_sta_ssid();
      json += "\"";

      json += ",\"ip\":\"";
      json += wifi_sta_ip();
      json += "\"";
    }

    json += "}";

    server.send(200, "application/json", json);
  });

  // ---------------------------------------------------------------------------
  // Wi-Fi scan (AP-only)
  // ---------------------------------------------------------------------------
  server.on("/api/wifi/scan", HTTP_GET, [&] {
    if (!wifi_is_ap_mode()) {
      server.send(403, "text/plain", "Scan disabled");
      return;
    }
    server.send(200, "application/json", wifi_get_scan_json());
  });

  // ---------------------------------------------------------------------------
  // SD file list
  // ---------------------------------------------------------------------------
  server.on("/api/files", HTTP_GET, [&] {
    StaticJsonDocument<1024> j;
    JsonArray a = j.to<JsonArray>();

    File root = SD_MMC.open("/");
    File f;

    while ((f = root.openNextFile())) {
      if (!f.isDirectory()) {
        String name = f.name();
        int slash = name.lastIndexOf('/');
        if (slash >= 0) name = name.substring(slash + 1);

        if (!isUserFile(name)) {
          f.close();
          continue;
        }

        JsonObject o = a.createNestedObject();
        o["name"] = name;
        o["size"] = f.size();
      }
      f.close();
    }

    sendJson(server, j);
  });

  // ---------------------------------------------------------------------------
  // File download
  // ---------------------------------------------------------------------------
  server.on("/api/file", HTTP_GET, [&] {

    if (!server.hasArg("name")) {
      server.send(400, "text/plain", "Missing name");
      return;
    }

    String name = server.arg("name");

    if (name.indexOf("..") >= 0 || name.indexOf('/') >= 0) {
      server.send(403, "text/plain", "Invalid filename");
      return;
    }

    if (!isUserFile(name)) {
      server.send(403, "text/plain", "Forbidden");
      return;
    }

    File f = SD_MMC.open("/Archive/" + name, FILE_READ);
    if (!f || f.isDirectory()) {
      server.send(404, "text/plain", "Not found");
      return;
    }

    server.sendHeader(
      "Content-Disposition",
      "attachment; filename=\"" + name + "\""
    );
    server.sendHeader("Cache-Control", "no-store");

    server.streamFile(f, "application/octet-stream");
    f.close();
  });

  // ---------------------------------------------------------------------------
  // Reboot
  // ---------------------------------------------------------------------------
  server.on("/api/reboot", HTTP_POST, [&] {
    server.send(200, "text/plain", "Rebooting");
    delay(200);
    ESP.restart();
  });

  // ---------------------------------------------------------------------------
  // SPA fallback
  // ---------------------------------------------------------------------------
  server.onNotFound([&] {
    server.send(200, "text/html", PAGE_CONFIG_APP);
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
// STA helpers (used by display + logs + UI)
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
  return wifi_sta_connected()
           ? WiFi.localIP().toString()
           : "";
}
