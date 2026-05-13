#include "Config/config_json.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#include "Core/log.h"
#include "Config/config_types.h"

namespace {

void loadStringField(JsonDocument& doc, const char* key, char* dest, size_t destSize)
{
  if (!doc.containsKey(key)) return;
  strlcpy(dest, doc[key] | "", destSize);
}

void loadNonEmptyStringField(JsonDocument& doc, const char* key, char* dest, size_t destSize)
{
  const char* value = doc[key];
  if (value && value[0]) {
    strlcpy(dest, value, destSize);
  }
}

void writeNonEmptyStringField(JsonDocument& doc, const char* key, const char* value)
{
  if (value[0]) {
    doc[key] = value;
  }
}

}

bool config_load_json(File& file)
{
  StaticJsonDocument<1536> doc;
  if (deserializeJson(doc, file)) {
    LOG_ERROR("CONFIG", "JSON parse failed");
    return false;
  }

  config.cal_bat = doc["cal_bat"] | config.cal_bat;
  config.shutdown_voltage = doc["shutdown_voltage"] | config.shutdown_voltage;

  config.logUBX = doc["logUBX"] | config.logUBX;
  config.logSBP = true;
  config.timezone = doc["timezone"] | config.timezone;
  config.timezone_DST = doc["timezone_DST"] | config.timezone_DST;

  config.stat_2s = doc["stat_2s"] | config.stat_2s;
  config.stat_10s = doc["stat_10s"] | config.stat_10s;
  config.stat_alpha = doc["stat_alpha"] | config.stat_alpha;
  config.stat_nm = doc["stat_nm"] | config.stat_nm;
  config.stat_1h = doc["stat_1h"] | config.stat_1h;
  config.stat_distance = doc["stat_distance"] | config.stat_distance;

  loadStringField(doc, "Sleep_info1", config.Sleep_info1, sizeof(config.Sleep_info1));
  loadStringField(doc, "Sleep_info2", config.Sleep_info2, sizeof(config.Sleep_info2));

  loadNonEmptyStringField(doc, "phone_ssid", config.phone_ssid, sizeof(config.phone_ssid));
  loadNonEmptyStringField(doc, "phone_pass", config.phone_pass, sizeof(config.phone_pass));

  return true;
}

void config_write_json(File& file)
{
  StaticJsonDocument<1536> doc;

  doc["cal_bat"] = config.cal_bat;
  doc["shutdown_voltage"] = config.shutdown_voltage;

  doc["logUBX"] = config.logUBX;
  doc["logSBP"] = true;

  doc["timezone"] = config.timezone;
  doc["timezone_DST"] = config.timezone_DST;

  doc["stat_2s"] = config.stat_2s;
  doc["stat_10s"] = config.stat_10s;
  doc["stat_alpha"] = config.stat_alpha;
  doc["stat_nm"] = config.stat_nm;
  doc["stat_1h"] = config.stat_1h;
  doc["stat_distance"] = config.stat_distance;

  doc["Sleep_info1"] = config.Sleep_info1;
  doc["Sleep_info2"] = config.Sleep_info2;

  writeNonEmptyStringField(doc, "phone_ssid", config.phone_ssid);
  writeNonEmptyStringField(doc, "phone_pass", config.phone_pass);

  serializeJsonPretty(doc, file);
}
