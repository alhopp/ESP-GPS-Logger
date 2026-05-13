// ============================================================================
// web_server.cpp
// HTTP server wiring for ESP32 GPS Logger.
// ============================================================================

#include "Web/Server/web_server.h"

#include <Arduino.h>
#include <LittleFS.h>

#include "Core/log.h"
#include "Web/API/api_config.h"
#include "Web/API/api_status.h"
#include "Web/Files/web_files.h"
#include "Web/WiFi/wifi_manager.h"

#include <esp_system.h>

static WebServer server(80);
static bool webStarted = false;
static bool routesRegistered = false;
static uint32_t lastWebActivityMs = 0;

namespace {

void handleRoot()
{
  webserver_note_activity();
  const char* page = wifi_show_ap_page() ? "/ap.html" : "/index.html";

  File f = LittleFS.open(page, "r");
  if (!f) {
    server.send(404, "text/plain", "UI missing");
    return;
  }

  server.streamFile(f, "text/html");
  f.close();
}

void handleReboot()
{
  webserver_note_activity();
  server.send(200, "text/plain", "OK");
  delay(200);
  ESP.restart();
}

void sendNoContent()
{
  webserver_note_activity();
  server.send(204);
}

void registerStaticRoutes()
{
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/reboot", HTTP_POST, handleReboot);
  server.on("/favicon.ico", HTTP_GET, sendNoContent);
  server.on("/apple-touch-icon.png", HTTP_GET, sendNoContent);
  server.on("/apple-touch-icon-precomposed.png", HTTP_GET, sendNoContent);
  server.serveStatic("/", LittleFS, "/");
  server.onNotFound(sendNoContent);
}

void registerApiRoutes()
{
  registerConfigApi(server);
  registerStatusApi(server);
  registerFileEndpoints(server);
}

}

void webserver_start()
{
  if (webStarted) return;

  if (!routesRegistered) {
    registerApiRoutes();
    registerStaticRoutes();
    routesRegistered = true;
  }

  server.begin();
  webStarted = true;
  webserver_note_activity();
  LOG_WIFI("Web", "started (network active)");
}

void webserver_stop()
{
  if (webStarted) {
    server.stop();
  }
  webStarted = false;
  lastWebActivityMs = 0;
}

void webserver_loop()
{
  server.handleClient();
}

void webserver_note_activity()
{
  lastWebActivityMs = millis();
}

uint32_t webserver_last_activity_ms()
{
  return lastWebActivityMs;
}
