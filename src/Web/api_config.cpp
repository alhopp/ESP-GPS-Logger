#include "Web/api_config.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include "Config/config_manager.h"
#include "Core/build_config.h"
#include "Core/system_info.h"
#include "Storage/storage_manager.h"
#include "Web/web_json.h"
#include "Web/wifi_manager.h"

namespace {
constexpr size_t CONFIG_GET_JSON_BYTES = 3072;
constexpr size_t CONFIG_POST_JSON_BYTES = 2048;

void addWifiJson(JsonDocument& j)
{
  JsonObject wifi = j.createNestedObject("wifi");
  wifi["connected"] = WiFi.status() == WL_CONNECTED;
  wifi["ssid"] = WiFi.isConnected() ? WiFi.SSID() : "";
  wifi["ip"] = WiFi.isConnected() ? WiFi.localIP().toString() : "";
  wifi["phone_ssid"] = wifi_effective_phone_ssid();
  wifi["saved_phone_ssid"] = config.phone_ssid;
  wifi["phone_pass_set"] = wifi_effective_phone_password_set();
}

void addSystemJson(JsonDocument& j)
{
  JsonObject system = j.createNestedObject("system");
  system["gnss_module"] = systemInfo.gnss_module;
  system["gnss_mode"] = systemInfo.gnss_mode;
  system["dynamic_model"] = systemInfo.dynamic_model;
  system["sample_rate"] = systemInfo.sample_rate;
  system["storage_mb"] = storage_sd_total_mb();
  system["storage_used_mb"] = storage_sd_used_mb();
  system["storage_free_mb"] = storage_sd_free_mb();
  system["storage_bytes"] = storage_sd_total_bytes();
  system["storage_used_bytes"] = storage_sd_used_bytes();
  system["storage_free_bytes"] = storage_sd_free_bytes();
  system["storage_detected"] = storage_sd_available();
  system["software_version"] = systemInfo.software_version;
  system["display"] = systemInfo.display;
  system["cpu_freq"] = getCpuFrequencyMhz();
  system["speed_units"] = systemInfo.speed_units;
  system["cal_speed"] = systemInfo.cal_speed;
  system["simulator"] = build_gps_simulator_enabled();
  system["dev_wifi"] = build_dev_wifi_enabled();
  system["logging_enabled"] = LOG_ENABLED != 0;
  system["wifi_connected"] = WiFi.status() == WL_CONNECTED;
  system["wifi_ssid"] = WiFi.isConnected() ? WiFi.SSID() : "";
  system["wifi_ip"] = WiFi.isConnected() ? WiFi.localIP().toString() : "";
  system["wifi_phone_ssid"] = wifi_effective_phone_ssid();
}

void addConfigJson(JsonDocument& j)
{
  JsonObject configJ = j.createNestedObject("config");
  configJ["timezone"] = config.timezone;
  configJ["timezone_DST"] = config.timezone_DST;
}

void addPowerJson(JsonDocument& j)
{
  JsonObject power = j.createNestedObject("power");
  power["cal_bat"] = config.cal_bat;
}

void addLoggingJson(JsonDocument& j)
{
  JsonObject logging = j.createNestedObject("logging");
  logging["logUBX"] = config.logUBX;
  logging["logSBP"] = true;
}

void addUiJson(JsonDocument& j)
{
  JsonObject ui = j.createNestedObject("ui");
  ui["Sleep_info1"] = config.Sleep_info1;
  ui["Sleep_info2"] = config.Sleep_info2;
}

void addStatsConfigJson(JsonDocument& j)
{
  JsonObject stats = j.createNestedObject("stats");
  stats["s2"] = config.stat_2s;
  stats["s10"] = config.stat_10s;
  stats["alpha"] = config.stat_alpha;
  stats["nm"] = config.stat_nm;
  stats["h1"] = config.stat_1h;
  stats["distance"] = config.stat_distance;
}

void applyWifiJson(JsonDocument& j)
{
  if (!j["wifi"]) return;

  const char* s;
  if ((s = j["wifi"]["phone_ssid"]) != nullptr) {
    strlcpy(config.phone_ssid, s, sizeof(config.phone_ssid));
  }
  if ((s = j["wifi"]["phone_pass"]) != nullptr) {
    strlcpy(config.phone_pass, s, sizeof(config.phone_pass));
  }
}

void applyLoggingJson(JsonDocument& j)
{
  if (!j["logging"]) return;

  if (j["logging"]["logUBX"] != nullptr) config.logUBX = j["logging"]["logUBX"];
  config.logSBP = true;
}

void applyStatsConfigJson(JsonDocument& j)
{
  if (!j["stats"]) return;

  if (j["stats"]["s2"] != nullptr) config.stat_2s = j["stats"]["s2"];
  if (j["stats"]["s10"] != nullptr) config.stat_10s = j["stats"]["s10"];
  if (j["stats"]["alpha"] != nullptr) config.stat_alpha = j["stats"]["alpha"];
  if (j["stats"]["nm"] != nullptr) config.stat_nm = j["stats"]["nm"];
  if (j["stats"]["h1"] != nullptr) config.stat_1h = j["stats"]["h1"];
  if (j["stats"]["distance"] != nullptr) config.stat_distance = j["stats"]["distance"];
}

void applyUiJson(JsonDocument& j)
{
  if (!j["ui"]) return;

  const char* s;
  if ((s = j["ui"]["Sleep_info1"]) != nullptr) {
    strlcpy(config.Sleep_info1, s, sizeof(config.Sleep_info1));
  }
  if ((s = j["ui"]["Sleep_info2"]) != nullptr) {
    strlcpy(config.Sleep_info2, s, sizeof(config.Sleep_info2));
  }
}
}

void registerConfigApi(WebServer& server)
{
  server.on("/api/config", HTTP_GET, [&server] {
    DynamicJsonDocument j(CONFIG_GET_JSON_BYTES);

    addWifiJson(j);
    addSystemJson(j);
    addConfigJson(j);
    addPowerJson(j);
    addLoggingJson(j);
    addUiJson(j);
    addStatsConfigJson(j);

    web_send_json(server, j);
  });

  server.on("/api/config", HTTP_POST, [&server] {
    DynamicJsonDocument j(CONFIG_POST_JSON_BYTES);
    if (deserializeJson(j, server.arg("plain"))) {
      server.send(400, "text/plain", "Bad JSON");
      return;
    }

    applyWifiJson(j);
    applyLoggingJson(j);
    applyStatsConfigJson(j);
    applyUiJson(j);

    saveConfig();
    server.send(200, "text/plain", "OK");
  });
}
