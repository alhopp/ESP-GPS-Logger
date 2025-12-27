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

// ------------------------------------------------------------
// helpers
// ------------------------------------------------------------

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

// ------------------------------------------------------------
// start server
// ------------------------------------------------------------

void webserver_start(WebServer &server)
{
  if (webStarted) return;

  // ------------------------------------------------------------
  // SPA root
  // ------------------------------------------------------------
  server.on("/", HTTP_GET, [&] {
    server.send(200, "text/html", PAGE_CONFIG_APP);
  });

  // ------------------------------------------------------------
  // GET config
  // ------------------------------------------------------------
  server.on("/api/config", HTTP_GET, [&] {
    StaticJsonDocument<1024> j;

    j["wifi"]["ssid"] = config.ssid;

    j["system"]["cpu_freq"]     = config.cpu_freq;
    j["system"]["timezone"]     = config.timezone;
    j["system"]["timezone_dst"] = config.timezone_DST;

    j["gps"]["sample_rate"]   = config.sample_rate;
    j["gps"]["gnss"]          = config.gnss;
    j["gps"]["dynamic_model"] = config.dynamic_model;
    j["gps"]["cal_speed"]     = config.cal_speed;

    j["power"]["shutdown_voltage"] = config.shutdown_voltage;
    j["power"]["bat_choice"]       = config.bat_choice;

    sendJson(server, j);
  });

  // ------------------------------------------------------------
  // POST config
  // ------------------------------------------------------------
  server.on("/api/config", HTTP_POST, [] {}, [&] {

    StaticJsonDocument<1024> j;
    if (deserializeJson(j, server.arg("plain"))) {
      server.send(400, "text/plain", "Bad JSON");
      return;
    }

    if (j["wifi"]["ssid"]) {
      strncpy(config.ssid, j["wifi"]["ssid"], sizeof(config.ssid) - 1);
      config.ssid[sizeof(config.ssid) - 1] = '\0';
    }

    if (j["system"]) {
      config.cpu_freq     = j["system"]["cpu_freq"]     | config.cpu_freq;
      config.timezone     = j["system"]["timezone"]     | config.timezone;
      config.timezone_DST = j["system"]["timezone_dst"] | config.timezone_DST;
    }

    if (j["gps"]) {
      config.sample_rate   = j["gps"]["sample_rate"]   | config.sample_rate;
      config.gnss          = j["gps"]["gnss"]          | config.gnss;
      config.dynamic_model = j["gps"]["dynamic_model"] | config.dynamic_model;
      config.cal_speed     = j["gps"]["cal_speed"]     | config.cal_speed;
    }

    if (j["power"]) {
      config.shutdown_voltage = j["power"]["shutdown_voltage"] | config.shutdown_voltage;
      config.bat_choice       = j["power"]["bat_choice"]       | config.bat_choice;
    }

    saveConfig();
    server.send(200, "text/plain", "OK");
  });

  // ------------------------------------------------------------
  // Wi-Fi scan
  // ------------------------------------------------------------
  server.on("/api/wifi/scan", HTTP_GET, [&] {

    StaticJsonDocument<2048> j;
    JsonArray a = j.to<JsonArray>();

    int n = WiFi.scanNetworks(false, false); // sync scan, no hidden

    for (int i = 0; i < n; i++) {
      String ssid = WiFi.SSID(i);
      if (ssid.isEmpty()) continue;

      // Deduplicate
      bool seen = false;
      for (JsonObject o : a) {
        if (o["ssid"] == ssid) { seen = true; break; }
      }
      if (seen) continue;

      JsonObject o = a.createNestedObject();
      o["ssid"]   = ssid;
      o["rssi"]   = WiFi.RSSI(i);
      o["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }

    WiFi.scanDelete(); // free heap
    sendJson(server, j);
  });

  // ------------------------------------------------------------
  // SD file list (filtered)
  // ------------------------------------------------------------
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

        for (size_t i = 0; i < name.length(); i++) {
          if (name[i] < 32 || name[i] > 126) name[i] = '_';
        }

        JsonObject o = a.createNestedObject();
        o["name"] = name;
        o["size"] = f.size();
      }

      f.close();
    }

    sendJson(server, j);
  });

  // ------------------------------------------------------------
  // SD file download
  // ------------------------------------------------------------
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

    String path = "/Archive/" + name;
    File f = SD_MMC.open(path, FILE_READ);

    if (!f || f.isDirectory()) {
      server.send(404, "text/plain", "Not found");
      return;
    }

    String ct = "application/octet-stream";
    if (name.endsWith(".json")) ct = "application/json";
    else if (name.endsWith(".txt")) ct = "text/plain";
    else if (name.endsWith(".gpx")) ct = "application/gpx+xml";

    server.sendHeader("Content-Disposition",
      "attachment; filename=\"" + name + "\"");
    server.sendHeader("Cache-Control", "no-store");

    server.streamFile(f, ct);
    f.close();
  });

  // ------------------------------------------------------------
  // reboot
  // ------------------------------------------------------------
  server.on("/api/reboot", HTTP_POST, [&] {
    server.send(200, "text/plain", "Rebooting");
    delay(200);
    ESP.restart();
  });

  // ------------------------------------------------------------
  // SPA fallback
  // ------------------------------------------------------------
  server.onNotFound([&] {
    server.send(200, "text/html", PAGE_CONFIG_APP);
  });

  server.begin();
  webStarted = true;

  LOG_WIFI("Web", "started");
}

// ------------------------------------------------------------
// stop
// ------------------------------------------------------------

void webserver_stop()
{
  webStarted = false;
}
