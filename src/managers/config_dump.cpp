#include "managers/config_dump.h"

#include <Arduino.h>

#include "core/system_info.h"
#include "managers/config_types.h"

void config_dump()
{
  Serial.println();
  Serial.println("[CONFIG ] ===== Loaded from JSON =====");

  Serial.print("[CONFIG ] cal_bat          = "); Serial.println(config.cal_bat);
  Serial.print("[CONFIG ] shutdown_voltage = "); Serial.println(config.shutdown_voltage);
  Serial.print("[CONFIG ] bar_length       = "); Serial.println(config.bar_length);
  Serial.print("[CONFIG ] logUBX           = "); Serial.println(config.logUBX);
  Serial.print("[CONFIG ] logSBP           = "); Serial.println(config.logSBP);
  Serial.print("[CONFIG ] timezone         = "); Serial.println(config.timezone);
  Serial.print("[CONFIG ] timezone_DST     = "); Serial.println(config.timezone_DST);
  Serial.print("[CONFIG ] track_distance   = "); Serial.println(config.track_distance);

  Serial.println("[CONFIG ] Performance screens");
  Serial.print("[CONFIG ]   2s             = "); Serial.println(config.stat_2s);
  Serial.print("[CONFIG ]   10s            = "); Serial.println(config.stat_10s);
  Serial.print("[CONFIG ]   alpha          = "); Serial.println(config.stat_alpha);
  Serial.print("[CONFIG ]   nm             = "); Serial.println(config.stat_nm);
  Serial.print("[CONFIG ]   1h             = "); Serial.println(config.stat_1h);
  Serial.print("[CONFIG ]   distance       = "); Serial.println(config.stat_distance);

  Serial.print("[CONFIG ] Sleep_info1      = "); Serial.println(config.Sleep_info1);
  Serial.print("[CONFIG ] Sleep_info2      = "); Serial.println(config.Sleep_info2);

  Serial.println("[CONFIG ] Wi-Fi");
  Serial.print("[CONFIG ]   home_ssid      = ");
  Serial.println(config.home_ssid[0] ? config.home_ssid : "(not set)");
  Serial.print("[CONFIG ]   home_pass      = ");
  Serial.println(config.home_pass[0] ? "***" : "(not set)");
  Serial.print("[CONFIG ]   phone_ssid     = ");
  Serial.println(config.phone_ssid[0] ? config.phone_ssid : "(not set)");
  Serial.print("[CONFIG ]   phone_pass     = ");
  Serial.println(config.phone_pass[0] ? "***" : "(not set)");

  Serial.println();
  Serial.println("[SYSTEM ] ===== Static system info =====");

  Serial.print("[SYSTEM ] gnss_module      = "); Serial.println(systemInfo.gnss_module);
  Serial.print("[SYSTEM ] gnss_mode        = "); Serial.println(systemInfo.gnss_mode);
  Serial.print("[SYSTEM ] dynamic_model    = "); Serial.println(systemInfo.dynamic_model);
  Serial.print("[SYSTEM ] sample_rate      = "); Serial.println(systemInfo.sample_rate);
  Serial.print("[SYSTEM ] speed_units      = "); Serial.println(systemInfo.speed_units);
  Serial.print("[SYSTEM ] cal_speed        = "); Serial.println(systemInfo.cal_speed);
  Serial.print("[SYSTEM ] storage_mb       = "); Serial.println(systemInfo.storage_mb);
  Serial.print("[SYSTEM ] display          = "); Serial.println(systemInfo.display);
  Serial.print("[SYSTEM ] cpu_freq         = "); Serial.println(systemInfo.cpu_freq);
  Serial.print("[SYSTEM ] software_version = "); Serial.println(systemInfo.software_version);

  Serial.println("=======================================");
  Serial.println();
}
