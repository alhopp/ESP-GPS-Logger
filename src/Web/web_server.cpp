// ============================================================================
// web_server.cpp
// HTTP server wiring for ESP32 GPS Logger.
// ============================================================================

#include "Web/web_server.h"

#include <Arduino.h>
#include <LittleFS.h>

#include "Core/log.h"
#include "Web/api_config.h"
#include "Web/api_status.h"
#include "Web/web_files.h"
#include "Web/wifi_manager.h"

#include <esp_system.h>

static WebServer server(80);
static bool webStarted = false;

namespace {

void handleRoot()
{
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
  server.send(200, "text/plain", "OK");
  delay(200);
  ESP.restart();
}

void sendNoContent()
{
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

  registerApiRoutes();
  registerStaticRoutes();

  server.begin();
  webStarted = true;
  LOG_WIFI("Web", "started (network active)");
}

void webserver_stop()
{
  if (webStarted) {
    server.stop();
  }
  webStarted = false;
}

void webserver_loop()
{
  server.handleClient();
}
