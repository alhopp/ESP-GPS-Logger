#include "web/api_status.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include "Storage/storage_manager.h"
#include "Ublox/Ublox.h"
#include "core/Globals.h"
#include "core/build_config.h"
#include "core/system_mode.h"
#include "session/logging_session.h"

namespace {
void sendJson(WebServer& server, JsonDocument& doc)
{
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}
}

void registerStatusApi(WebServer& server)
{
  server.on("/api/netstatus", HTTP_GET, [&server] {
    StaticJsonDocument<256> j;
    j["connected"] = WiFi.status() == WL_CONNECTED;
    j["ssid"] = WiFi.SSID();
    j["ip"] = WiFi.localIP().toString();
    sendJson(server, j);
  });

  server.on("/api/status", HTTP_GET, [&server] {
    StaticJsonDocument<512> j;
    j["mode"] = modeToString(getMode());
    j["session_active"] = logging_session_active();
    j["gps_signal_ok"] = GPS_Signal_OK;
    j["satellites"] = ubxMessage.navPvt.numSV;
    j["fix_type"] = ubxMessage.navPvt.fixType;
    j["sd_ok"] = sdOK;
    j["littlefs_ok"] = LITTLEFS_OK;
    j["storage_shutting_down"] = storage_shutting_down;
    j["simulator"] = build_gps_simulator_enabled();
    j["wifi_connected"] = WiFi.status() == WL_CONNECTED;
    sendJson(server, j);
  });
}

