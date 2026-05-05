#include "web/api_config.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include "Config/config_manager.h"
#include "Core/system_info.h"

namespace {
void sendJson(WebServer& server, JsonDocument& doc)
{
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}
}

void registerConfigApi(WebServer& server)
{
  server.on("/api/config", HTTP_GET, [&server] {
    DynamicJsonDocument j(3072);

    JsonObject wifi = j.createNestedObject("wifi");
    wifi["connected"] = WiFi.status() == WL_CONNECTED;
    wifi["ssid"] = WiFi.isConnected() ? WiFi.SSID() : "";
    wifi["ip"] = WiFi.isConnected() ? WiFi.localIP().toString() : "";
    wifi["phone_ssid"] = config.phone_ssid;
    wifi["phone_pass_set"] = config.phone_pass[0] ? true : false;

    JsonObject system = j.createNestedObject("system");
    system["gnss_module"] = systemInfo.gnss_module;
    system["gnss_mode"] = systemInfo.gnss_mode;
    system["dynamic_model"] = systemInfo.dynamic_model;
    system["sample_rate"] = systemInfo.sample_rate;
    system["storage_mb"] = systemInfo.storage_mb;
    system["software_version"] = systemInfo.software_version;
    system["display"] = systemInfo.display;
    system["cpu_freq"] = systemInfo.cpu_freq;
    system["speed_units"] = systemInfo.speed_units;
    system["cal_speed"] = systemInfo.cal_speed;

    JsonObject configJ = j.createNestedObject("config");
    configJ["timezone"] = config.timezone;
    configJ["timezone_DST"] = config.timezone_DST;

    JsonObject power = j.createNestedObject("power");
    power["cal_bat"] = config.cal_bat;

    JsonObject logging = j.createNestedObject("logging");
    logging["logUBX"] = config.logUBX;
    logging["logSBP"] = config.logSBP;

    JsonObject ui = j.createNestedObject("ui");
    ui["bar_length"] = config.bar_length;
    ui["Sleep_info1"] = config.Sleep_info1;
    ui["Sleep_info2"] = config.Sleep_info2;

    JsonObject stats = j.createNestedObject("stats");
    stats["s2"] = config.stat_2s;
    stats["s10"] = config.stat_10s;
    stats["alpha"] = config.stat_alpha;
    stats["nm"] = config.stat_nm;
    stats["h1"] = config.stat_1h;
    stats["distance"] = config.stat_distance;

    sendJson(server, j);
  });

  server.on("/api/config", HTTP_POST, [&server] {
    DynamicJsonDocument j(2048);
    if (deserializeJson(j, server.arg("plain"))) {
      server.send(400, "text/plain", "Bad JSON");
      return;
    }

    if (j["wifi"]) {
      const char* s;
      if ((s = j["wifi"]["phone_ssid"]) != nullptr) {
        strlcpy(config.phone_ssid, s, sizeof(config.phone_ssid));
      }
      if ((s = j["wifi"]["phone_pass"]) != nullptr) {
        strlcpy(config.phone_pass, s, sizeof(config.phone_pass));
      }
    }

    if (j["logging"]) {
      if (j["logging"]["logTXT"] != nullptr) config.track_distance = j["logging"]["logTXT"];
      if (j["logging"]["logUBX"] != nullptr) config.logUBX = j["logging"]["logUBX"];
      if (j["logging"]["logSBP"] != nullptr) config.logSBP = j["logging"]["logSBP"];
    }

    if (j["stats"]) {
      if (j["stats"]["s2"] != nullptr) config.stat_2s = j["stats"]["s2"];
      if (j["stats"]["s10"] != nullptr) config.stat_10s = j["stats"]["s10"];
      if (j["stats"]["alpha"] != nullptr) config.stat_alpha = j["stats"]["alpha"];
      if (j["stats"]["nm"] != nullptr) config.stat_nm = j["stats"]["nm"];
      if (j["stats"]["h1"] != nullptr) config.stat_1h = j["stats"]["h1"];
      if (j["stats"]["distance"] != nullptr) config.stat_distance = j["stats"]["distance"];
    }

    if (j["ui"]) {
      const char* s;
      if ((s = j["ui"]["Sleep_info1"]) != nullptr) {
        strlcpy(config.Sleep_info1, s, sizeof(config.Sleep_info1));
      }
      if ((s = j["ui"]["Sleep_info2"]) != nullptr) {
        strlcpy(config.Sleep_info2, s, sizeof(config.Sleep_info2));
      }
    }

    saveConfig();
    server.send(200, "text/plain", "OK");
  });
}
