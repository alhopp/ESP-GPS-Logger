// ============================================================================
// web_server.cpp
//
// HTTP server wiring for ESP32 GPS Logger (CONFIG / SoftAP only)
//
// Responsibilities:
// - Serve the single-page configuration UI (LittleFS)
// - Expose JSON APIs for config, Wi-Fi, system info
// - Delegate all SD / log file handling to web_files.cpp
//
// Notes:
// - Server is guarded against double start
// - Nothing here runs in LOGGING mode
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

// ------------0-----------------------------------------------------------------
// Server lifecycle guard
// -----------------------------------------------------------------------------
static bool webStarted=false;

// -----------------------------------------------------------------------------
// Small helper to serialize + send JSON consistently
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
  // GET /api/config
  // - Returns current runtime + persisted configuration
  // - Used to populate the SPA on load
  // ---------------------------------------------------------------------------
  server.on("/api/config",HTTP_GET,[&]{
  DynamicJsonDocument j(2048);

  j["wifi"]["ssid"]                    = wifi_get_saved_ssid();

  j["system"]["cpu_freq"]              = config.cpu_freq;
  j["system"]["timezone"]              = config.timezone;
  j["system"]["timezone_DST"]          = config.timezone_DST;
  j["system"]["gnss_module"]           = systemInfo.gnss_module;
  j["system"]["storage_mb"]            = systemInfo.storage_mb;
  j["system"]["software_version"]      = systemInfo.software_version;
  j["system"]["display"]               = systemInfo.display;

  j["gps"]["speed_units"]              = systemInfo.speed_units;
  j["gps"]["sample_rate"]              = systemInfo.sample_rate;
  j["gps"]["gnss"]                     = systemInfo.gnss_mode;
  j["gps"]["dynamic_model"]            = systemInfo.dynamic_model;
  j["gps"]["cal_speed"]                = config.cal_speed;
  j["gps"]["stat_speed"]               = config.stat_speed;


  j["power"]["cal_bat"]                = config.cal_bat;

  j["logging"]["track_distance"]       = config.track_distance;
  j["logging"]["archive_days"]         = config.archive_days;
  j["logging"]["file_date_time"]       = config.file_date_time;
  j["logging"]["logTXT"]               = config.logTXT;
  j["logging"]["logUBX"]               = config.logUBX;
  j["logging"]["logSBP"]               = config.logSBP;

  j["ui"]["bar_length"]                = config.bar_length;
  j["ui"]["sleep_off_screen"]          = config.sleep_off_screen;

  j["ui"]["Stat_screens"]              = config.Stat_screens;
  j["ui"]["speed_screen"]              = config.speed_screen;
  j["ui"]["stat_screen"]               = config.stat_screen;
  j["ui"]["gpio12_screen"]             = config.gpio12_screen;
  j["ui"]["Sleep_info"]                = config.Sleep_info;

  sendJson(server,j);
  });


  // ---------------------------------------------------------------------------
  // POST /api/config
  // - Accepts partial config updates from the UI
  // - Writes to persistent storage
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
      if(j["logging"]["logUBX"]!=nullptr) config.logUBX=j["logging"]["logUBX"];
      if(j["logging"]["logSBP"]!=nullptr) config.logSBP=j["logging"]["logSBP"];
    }

    saveConfig();
    server.send(200,"text/plain","OK");
  });

  // ---------------------------------------------------------------------------
  // GET /api/netstatus
  // - Lightweight Wi-Fi status polling for the UI
  // ---------------------------------------------------------------------------
  server.on("/api/netstatus",HTTP_GET,[&]{
    StaticJsonDocument<256> j;
    j["sta"]=wifi_sta_connected();
    if(wifi_sta_connected()){
      j["ssid"]=wifi_sta_ssid();
      j["ip"]=wifi_sta_ip();
    }
    sendJson(server,j);
  });

  // ---------------------------------------------------------------------------
  // POST /api/wifi/connect
  // - Triggers STA connection attempt
  // ---------------------------------------------------------------------------
  server.on("/api/wifi/connect",HTTP_POST,[&]{
    server.send(200,"text/plain","OK");
    delay(50);
    wifi_start_sta();
  });

  // ---------------------------------------------------------------------------
  // File APIs (SD listing, download, delete)
  // ---------------------------------------------------------------------------
  registerFileEndpoints(server);

  // ---------------------------------------------------------------------------
  // Root page (explicit to ensure index.html always works)
  // ---------------------------------------------------------------------------
  server.on("/",HTTP_GET,[&]{
    File f=LittleFS.open("/index.html","r");
    if(!f){ server.send(404,"text/plain","index.html missing"); return; }
    server.streamFile(f,"text/html"); f.close();
  });

  // ---------------------------------------------------------------------------
  // Static assets (CSS / JS / images)
  // ---------------------------------------------------------------------------
  server.serveStatic("/",LittleFS,"/");

  // ---------------------------------------------------------------------------
  // Browser noise suppression (keeps logs clean)
  // ---------------------------------------------------------------------------
  server.on("/favicon.ico",HTTP_GET,[]{});
  server.on("/apple-touch-icon.png",HTTP_GET,[]{});
  server.on("/apple-touch-icon-precomposed.png",HTTP_GET,[]{});
  server.on("/manifest.json",HTTP_GET,[]{});
  server.on("/robots.txt",HTTP_GET,[]{});
  server.on("/service-worker.js",HTTP_GET,[]{});

  // ---------------------------------------------------------------------------
  // Catch-all (no redirects, no surprises)
  // ---------------------------------------------------------------------------
  server.onNotFound([&]{ server.send(204); });

  server.begin();
  webStarted=true;
  LOG_WIFI("Web","started");
}

// -----------------------------------------------------------------------------
// Stop server (used when leaving CONFIG mode)
// -----------------------------------------------------------------------------
void webserver_stop(){ webStarted=false; }

// -----------------------------------------------------------------------------
// Wi-Fi STA helpers (used by APIs above)
// -----------------------------------------------------------------------------
bool wifi_sta_connected(){ return WiFi.status()==WL_CONNECTED; }
String wifi_sta_ssid(){ return wifi_sta_connected()?WiFi.SSID():""; }
String wifi_sta_ip(){ return wifi_sta_connected()?WiFi.localIP().toString():""; }
