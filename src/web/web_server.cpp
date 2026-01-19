// ============================================================================
// web_server.cpp
// HTTP server wiring for ESP32 GPS Logger (CONFIG / SoftAP only)
//
// - Serves SPA UI from LittleFS
// - Exposes JSON APIs (config, Wi-Fi, system)
// - Delegates SD/log file handling to web_files.cpp
// - Guarded against double start; never runs in LOGGING mode
// ============================================================================

#include "web/web_server.h"
#include "web/web_files.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <LittleFS.h>

#include "config_manager.h"
#include "web/wifi_manager.h"
#include "system_info.h"
#include "Definitions.h"

#include <esp_system.h>



// -----------------------------------------------------------------------------
// Server lifecycle guard
// -----------------------------------------------------------------------------
static bool webStarted=false;

// -----------------------------------------------------------------------------
// JSON send helper (consistent response formatting)
// -----------------------------------------------------------------------------
static void sendJson(WebServer &s,JsonDocument &doc){
  String out; serializeJson(doc,out);
  s.send(200,"application/json",out);
}

// -----------------------------------------------------------------------------
// Start HTTP server (CONFIG / SoftAP mode only)
// -----------------------------------------------------------------------------
void webserver_start(WebServer &server)
{
  if(webStarted) return;

  // ---------------------------------------------------------------------------
  // GET /api/config → populate SPA on load
  // - config      : persisted user config (LittleFS)
  // - systemInfo  : compile-time / runtime system facts
  // ---------------------------------------------------------------------------
  server.on("/api/config", HTTP_GET, [&] {
    DynamicJsonDocument j(3072);   // 👈 small safety margin

    // ---------------- Wi-Fi ----------------
    JsonObject wifi = j.createNestedObject("wifi");
    wifi["ssid"] = wifi_get_saved_ssid();

    // ---------------- System (static / runtime) ----------------
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

    // ---------------- Config (user editable) ----------------
    JsonObject configJ         = j.createNestedObject("config");
    configJ["timezone"]        = config.timezone;
    configJ["timezone_DST"]    = config.timezone_DST;

    JsonObject gps = j.createNestedObject("gps");

    JsonObject power = j.createNestedObject("power");
    power["cal_bat"] = config.cal_bat;

    JsonObject logging = j.createNestedObject("logging");
    logging["logTXT"]          = config.track_distance;
    logging["logUBX"]          = config.logUBX;
    logging["logSBP"]          = config.logSBP;


    JsonObject ui = j.createNestedObject("ui");
    ui["bar_length"]           = config.bar_length;
    ui["Sleep_info"]           = config.Sleep_info;

    // ---------------- Performance screens ----------------
    JsonObject stats = j.createNestedObject("stats");
    stats["s2"]                = config.stat_2s;
    stats["s10"]               = config.stat_10s;
    stats["alpha"]             = config.stat_alpha;
    stats["nm"]                = config.stat_nm;
    stats["h1"]                = config.stat_1h;
    stats["distance"]          = config.stat_distance;

    sendJson(server, j);
  });


  // ---------------------------------------------------------------------------
  // POST /api/config → partial config updates from UI
  // ---------------------------------------------------------------------------
  server.on("/api/config",HTTP_POST,[&]{
    DynamicJsonDocument j(2048);
    if(deserializeJson(j,server.arg("plain"))){
      server.send(400,"text/plain","Bad JSON"); return;
    }

    if(j["wifi"]["ssid"]){
      String ssid=j["wifi"]["ssid"].as<const char*>(), pass;
      if(j["wifi"]["password"]) pass=j["wifi"]["password"].as<const char*>();
      wifi_set_credentials(ssid,pass);
    }

   if(j["logging"]){
      if(j["logging"]["logTXT"] != nullptr) config.track_distance = j["logging"]["logTXT"];
      if(j["logging"]["logUBX"] != nullptr) config.logUBX = j["logging"]["logUBX"];
      if(j["logging"]["logSBP"] != nullptr) config.logSBP = j["logging"]["logSBP"];
    }


    if (j["stats"]) {
      if (j["stats"]["s2"]        != nullptr) config.stat_2s        = j["stats"]["s2"];
      if (j["stats"]["s10"]       != nullptr) config.stat_10s       = j["stats"]["s10"];
      if (j["stats"]["alpha"]     != nullptr) config.stat_alpha     = j["stats"]["alpha"];
      if (j["stats"]["nm"]        != nullptr) config.stat_nm        = j["stats"]["nm"];
      if (j["stats"]["h1"]        != nullptr) config.stat_1h        = j["stats"]["h1"];
      if (j["stats"]["distance"]  != nullptr) config.stat_distance  = j["stats"]["distance"];
    }

    if (j["ui"]["Sleep_info"] != nullptr)
      strlcpy(config.Sleep_info,j["ui"]["Sleep_info"].as<const char*>(),sizeof(config.Sleep_info));

    saveConfig();
    server.send(200,"text/plain","OK");
  });

  // ---------------------------------------------------------------------------
  // GET /api/netstatus → lightweight Wi-Fi polling
  // ---------------------------------------------------------------------------
  server.on("/api/netstatus",HTTP_GET,[&]{
    StaticJsonDocument<256> j;
    j["sta"]=wifi_sta_connected();
    if(wifi_sta_connected()){
      j["ssid"]=wifi_sta_ssid();
      j["ip"]  =wifi_sta_ip();
    }
    sendJson(server,j);
  });

  // ---------------------------------------------------------------------------
  // POST /api/wifi/connect → trigger STA connection
  // ---------------------------------------------------------------------------
  server.on("/api/wifi/connect",HTTP_POST,[&]{
    server.send(200,"text/plain","OK");
    delay(50); wifi_start_sta();
  });

  // ---------------------------------------------------------------------------
  // SD / log file APIs
  // ---------------------------------------------------------------------------
  registerFileEndpoints(server);

  // ---------------------------------------------------------------------------
  // Root page (explicit index.html)
  // ---------------------------------------------------------------------------
  server.on("/",HTTP_GET,[&]{
    File f=LittleFS.open("/index.html","r");
    if(!f){ server.send(404,"text/plain","index.html missing"); return; }
    server.streamFile(f,"text/html"); f.close();
  });

// ---------------------------------------------------------------------------
// Browser noise suppression (optional)
// ---------------------------------------------------------------------------
server.on("/favicon.ico", HTTP_GET, [&]{ server.send(204); });
server.on("/apple-touch-icon.png", HTTP_GET, [&]{ server.send(204); });
server.on("/apple-touch-icon-precomposed.png", HTTP_GET, [&]{ server.send(204); });
server.on("/manifest.json", HTTP_GET, [&]{ server.send(204); });
server.on("/robots.txt", HTTP_GET, [&]{ server.send(204); });
server.on("/service-worker.js", HTTP_GET, [&]{ server.send(204); });


// ---------------------------------------------------------------------------
// onNotFound – MUST be before serveStatic
// ---------------------------------------------------------------------------
server.onNotFound([&]{
  String uri = server.uri();

  // ---- Offline tiles (silent) ----
  if (uri.startsWith("/tiles/")) {
    if (LittleFS.exists(uri)) {
      File f = LittleFS.open(uri, "r");
      server.streamFile(f, "image/jpeg");
      f.close();
    } else {
      server.send(204);   // silent success
    }
    return;
  }

  // ---- Everything else ----
  server.send(204);
});


// ---------------------------------------------------------------------------
// Static assets – MUST be last
// ---------------------------------------------------------------------------
server.serveStatic("/", LittleFS, "/");


  server.begin();
  webStarted=true;
  LOG_WIFI("Web","started");
}

// -----------------------------------------------------------------------------
// Stop server (leaving CONFIG mode)
// -----------------------------------------------------------------------------
void webserver_stop(){ webStarted=false; }

// -----------------------------------------------------------------------------
// Wi-Fi STA helpers (used by APIs)
// -----------------------------------------------------------------------------
bool   wifi_sta_connected(){ return WiFi.status()==WL_CONNECTED; }
String wifi_sta_ssid()     { return wifi_sta_connected()?WiFi.SSID():""; }
String wifi_sta_ip()       { return wifi_sta_connected()?WiFi.localIP().toString():""; }

