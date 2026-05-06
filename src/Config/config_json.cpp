#include "Config/config_json.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#include "Core/log.h"
#include "Config/config_types.h"

bool config_load_json(File& file)
{
  StaticJsonDocument<1536> doc;
  if (deserializeJson(doc, file)) {
    LOG_ERROR("CONFIG", "JSON parse failed");
    return false;
  }

  config.cal_bat = doc["cal_bat"] | config.cal_bat;
  config.shutdown_voltage = doc["shutdown_voltage"] | config.shutdown_voltage;
  config.bar_length = doc["bar_length"] | config.bar_length;

  config.logUBX = doc["logUBX"] | config.logUBX;
  config.logSBP = doc["logSBP"] | config.logSBP;
  config.timezone = doc["timezone"] | config.timezone;
  config.timezone_DST = doc["timezone_DST"] | config.timezone_DST;
  config.track_distance = doc["track_distance"] | config.track_distance;

  config.stat_2s = doc["stat_2s"] | config.stat_2s;
  config.stat_10s = doc["stat_10s"] | config.stat_10s;
  config.stat_alpha = doc["stat_alpha"] | config.stat_alpha;
  config.stat_nm = doc["stat_nm"] | config.stat_nm;
  config.stat_1h = doc["stat_1h"] | config.stat_1h;
  config.stat_distance = doc["stat_distance"] | config.stat_distance;

  if (doc.containsKey("Sleep_info1")) {
    strlcpy(config.Sleep_info1, doc["Sleep_info1"] | "", sizeof(config.Sleep_info1));
  }
  if (doc.containsKey("Sleep_info2")) {
    strlcpy(config.Sleep_info2, doc["Sleep_info2"] | "", sizeof(config.Sleep_info2));
  }

  const char* s;
  if ((s = doc["phone_ssid"]) && s[0]) {
    strlcpy(config.phone_ssid, s, sizeof(config.phone_ssid));
  }
  if ((s = doc["phone_pass"]) && s[0]) {
    strlcpy(config.phone_pass, s, sizeof(config.phone_pass));
  }

  return true;
}

void config_write_json(File& file)
{
  StaticJsonDocument<1536> doc;

  doc["cal_bat"] = config.cal_bat;
  doc["shutdown_voltage"] = config.shutdown_voltage;
  doc["bar_length"] = config.bar_length;

  doc["logUBX"] = config.logUBX;
  doc["logSBP"] = config.logSBP;

  doc["timezone"] = config.timezone;
  doc["timezone_DST"] = config.timezone_DST;
  doc["track_distance"] = config.track_distance;

  doc["stat_2s"] = config.stat_2s;
  doc["stat_10s"] = config.stat_10s;
  doc["stat_alpha"] = config.stat_alpha;
  doc["stat_nm"] = config.stat_nm;
  doc["stat_1h"] = config.stat_1h;
  doc["stat_distance"] = config.stat_distance;

  doc["Sleep_info1"] = config.Sleep_info1;
  doc["Sleep_info2"] = config.Sleep_info2;

  if (config.phone_ssid[0]) doc["phone_ssid"] = config.phone_ssid;
  if (config.phone_pass[0]) doc["phone_pass"] = config.phone_pass;

  serializeJsonPretty(doc, file);
}
