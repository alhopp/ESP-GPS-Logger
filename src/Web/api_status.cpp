#include "Web/api_status.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include "Storage/storage_manager.h"
#include "GPS/gps_runtime_state.h"
#include "GPS/Ublox/ublox_driver.h"
#include "GPS/Hardware/gps_manager.h"
#include "Core/Globals.h"
#include "Core/build_config.h"
#include "Core/system_mode.h"
#include "Logging/logging_session.h"
#include "Web/web_json.h"

void registerStatusApi(WebServer& server)
{
  server.on("/api/netstatus", HTTP_GET, [&server] {
    StaticJsonDocument<256> j;
    j["connected"] = WiFi.status() == WL_CONNECTED;
    j["ssid"] = WiFi.SSID();
    j["ip"] = WiFi.localIP().toString();
    web_send_json(server, j);
  });

  server.on("/api/status", HTTP_GET, [&server] {
    StaticJsonDocument<768> j;
    j["mode"] = modeToString(getMode());
    j["session_active"] = logging_session_active();
    j["gps_lifecycle"] = gps_lifecycle_state_name();
    j["gps_signal_ok"] = GPS_Signal_OK;
    j["satellites"] = ubxMessage.navPvt.numSV;
    j["fix_type"] = ubxMessage.navPvt.fixType;
    j["last_gps_age_ms"] = last_gps_msg ? millis() - last_gps_msg : -1;
    j["sd_ok"] = storage_sd_available();
    j["sd_mounted"] = storage_sd_mounted();
    j["littlefs_ok"] = storage_littlefs_available();
    j["storage_shutting_down"] = storage_is_shutting_down();
    j["simulator"] = build_gps_simulator_enabled();
    j["wifi_connected"] = WiFi.status() == WL_CONNECTED;
    web_send_json(server, j);
  });
}
