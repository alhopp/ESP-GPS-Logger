// ============================================================================
// api_status.cpp
//
// Registers lightweight runtime status endpoints for the config UI. Reports
// current mode, session state, GPS health, storage state, and network status.
// ============================================================================

#include "Web/API/api_status.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include "Storage/storage_manager.h"
#include "GPS/gps_runtime_state.h"
#include "GPS/Ublox/ublox_driver.h"
#include "GPS/Hardware/gps_manager.h"
#include "Core/Globals.h"
#include "Core/build_config.h"
#include "System/system_mode.h"
#include "Logging/logging_session.h"
#include "Web/Server/web_json.h"

namespace {

bool wifiConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

void addGpsStatus(JsonDocument& j)
{
  j["gps_lifecycle"] = gps_lifecycle_state_name();
  j["gps_signal_ok"] = GPS_Signal_OK;
  j["satellites"] = ubxMessage.navPvt.numSV;
  j["fix_type"] = ubxMessage.navPvt.fixType;
  j["last_gps_age_ms"] = last_gps_msg ? millis() - last_gps_msg : -1;
}

void addStorageStatus(JsonDocument& j)
{
  j["sd_ok"] = storage_sd_available();
  j["sd_mounted"] = storage_sd_mounted();
  j["littlefs_ok"] = storage_littlefs_available();
  j["storage_shutting_down"] = storage_is_shutting_down();
}

void handleNetStatus(WebServer& server)
{
  StaticJsonDocument<256> j;
  j["connected"] = wifiConnected();
  j["ssid"] = WiFi.SSID();
  j["ip"] = WiFi.localIP().toString();
  web_send_json(server, j);
}

void handleStatus(WebServer& server)
{
  StaticJsonDocument<768> j;
  j["mode"] = modeToString(getMode());
  j["session_active"] = logging_session_active();
  addGpsStatus(j);
  addStorageStatus(j);
  j["simulator"] = build_gps_simulator_enabled();
  j["wifi_connected"] = wifiConnected();
  web_send_json(server, j);
}

}

void registerStatusApi(WebServer& server)
{
  server.on("/api/netstatus", HTTP_GET, [&server] { handleNetStatus(server); });
  server.on("/api/status", HTTP_GET, [&server] { handleStatus(server); });
}
