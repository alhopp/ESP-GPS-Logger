#include "Config/config_json.h"

// ============================================================================
// Config JSON
//
// Compatibility layer for /config.txt. The constants below are the persisted
// key names; changing them would break existing device config files.
// ============================================================================

#include <Arduino.h>
#include <ArduinoJson.h>

#include "Core/log.h"
#include "Config/config_types.h"

namespace {
constexpr size_t CONFIG_JSON_BYTES = 1536;

constexpr const char* KEY_CAL_BAT = "cal_bat";
constexpr const char* KEY_SHUTDOWN_VOLTAGE = "shutdown_voltage";
constexpr const char* KEY_LOG_UBX = "logUBX";
constexpr const char* KEY_LOG_SBP = "logSBP";
constexpr const char* KEY_TIMEZONE = "timezone";
constexpr const char* KEY_TIMEZONE_DST = "timezone_DST";
constexpr const char* KEY_STAT_2S = "stat_2s";
constexpr const char* KEY_STAT_10S = "stat_10s";
constexpr const char* KEY_STAT_ALPHA = "stat_alpha";
constexpr const char* KEY_STAT_NM = "stat_nm";
constexpr const char* KEY_STAT_1H = "stat_1h";
constexpr const char* KEY_STAT_DISTANCE = "stat_distance";
constexpr const char* KEY_SLEEP_INFO1 = "Sleep_info1";
constexpr const char* KEY_SLEEP_INFO2 = "Sleep_info2";
constexpr const char* KEY_PHONE_SSID = "phone_ssid";
constexpr const char* KEY_PHONE_PASS = "phone_pass";

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
  StaticJsonDocument<CONFIG_JSON_BYTES> doc;
  if (deserializeJson(doc, file)) {
    LOG_ERROR("CONFIG", "JSON parse failed");
    return false;
  }

  config.cal_bat = doc[KEY_CAL_BAT] | config.cal_bat;
  config.shutdown_voltage = doc[KEY_SHUTDOWN_VOLTAGE] | config.shutdown_voltage;

  config.logUBX = doc[KEY_LOG_UBX] | config.logUBX;
  config.logSBP = true;
  config.timezone = doc[KEY_TIMEZONE] | config.timezone;
  config.timezone_DST = doc[KEY_TIMEZONE_DST] | config.timezone_DST;

  config.stat_2s = doc[KEY_STAT_2S] | config.stat_2s;
  config.stat_10s = doc[KEY_STAT_10S] | config.stat_10s;
  config.stat_alpha = doc[KEY_STAT_ALPHA] | config.stat_alpha;
  config.stat_nm = doc[KEY_STAT_NM] | config.stat_nm;
  config.stat_1h = doc[KEY_STAT_1H] | config.stat_1h;
  config.stat_distance = doc[KEY_STAT_DISTANCE] | config.stat_distance;

  loadStringField(doc, KEY_SLEEP_INFO1, config.Sleep_info1, sizeof(config.Sleep_info1));
  loadStringField(doc, KEY_SLEEP_INFO2, config.Sleep_info2, sizeof(config.Sleep_info2));

  loadNonEmptyStringField(doc, KEY_PHONE_SSID, config.phone_ssid, sizeof(config.phone_ssid));
  loadNonEmptyStringField(doc, KEY_PHONE_PASS, config.phone_pass, sizeof(config.phone_pass));

  return true;
}

void config_write_json(File& file)
{
  StaticJsonDocument<CONFIG_JSON_BYTES> doc;

  doc[KEY_CAL_BAT] = config.cal_bat;
  doc[KEY_SHUTDOWN_VOLTAGE] = config.shutdown_voltage;

  doc[KEY_LOG_UBX] = config.logUBX;
  doc[KEY_LOG_SBP] = true;

  doc[KEY_TIMEZONE] = config.timezone;
  doc[KEY_TIMEZONE_DST] = config.timezone_DST;

  doc[KEY_STAT_2S] = config.stat_2s;
  doc[KEY_STAT_10S] = config.stat_10s;
  doc[KEY_STAT_ALPHA] = config.stat_alpha;
  doc[KEY_STAT_NM] = config.stat_nm;
  doc[KEY_STAT_1H] = config.stat_1h;
  doc[KEY_STAT_DISTANCE] = config.stat_distance;

  doc[KEY_SLEEP_INFO1] = config.Sleep_info1;
  doc[KEY_SLEEP_INFO2] = config.Sleep_info2;

  writeNonEmptyStringField(doc, KEY_PHONE_SSID, config.phone_ssid);
  writeNonEmptyStringField(doc, KEY_PHONE_PASS, config.phone_pass);

  serializeJsonPretty(doc, file);
}
