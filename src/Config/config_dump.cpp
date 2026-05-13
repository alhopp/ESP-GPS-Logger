#include "Config/config_dump.h"

// ============================================================================
// Config diagnostics
//
// Emits the loaded config and live system facts to Serial. This is intentionally
// read-only and disabled for stats-only serial builds.
// ============================================================================

#include <Arduino.h>

#include "Core/build_config.h"
#include "Core/system_info.h"
#include "Config/config_types.h"
#include "Storage/storage_manager.h"

namespace {

void printField(const char* prefix, const char* label, const char* value)
{
  Serial.print(prefix);
  Serial.print(label);
  Serial.println(value);
}

void printField(const char* prefix, const char* label, float value)
{
  Serial.print(prefix);
  Serial.print(label);
  Serial.println(value);
}

void printField(const char* prefix, const char* label, bool value)
{
  Serial.print(prefix);
  Serial.print(label);
  Serial.println(value);
}

void printField(const char* prefix, const char* label, uint32_t value)
{
  Serial.print(prefix);
  Serial.print(label);
  Serial.println(value);
}

}

void config_dump()
{
#if STATS_ONLY_SERIAL
  return;
#endif

  Serial.println();
  Serial.println("[CONFIG ] ===== Loaded from JSON =====");

  printField("[CONFIG ] ", "cal_bat          = ", config.cal_bat);
  printField("[CONFIG ] ", "shutdown_voltage = ", config.shutdown_voltage);
  printField("[CONFIG ] ", "logUBX           = ", config.logUBX);
  printField("[CONFIG ] ", "logSBP           = ", true);
  printField("[CONFIG ] ", "timezone         = ", config.timezone);
  printField("[CONFIG ] ", "timezone_DST     = ", config.timezone_DST);

  Serial.println("[CONFIG ] Performance screens");
  printField("[CONFIG ] ", "  2s             = ", config.stat_2s);
  printField("[CONFIG ] ", "  10s            = ", config.stat_10s);
  printField("[CONFIG ] ", "  alpha          = ", config.stat_alpha);
  printField("[CONFIG ] ", "  nm             = ", config.stat_nm);
  printField("[CONFIG ] ", "  1h             = ", config.stat_1h);
  printField("[CONFIG ] ", "  distance       = ", config.stat_distance);

  printField("[CONFIG ] ", "Sleep_info1      = ", config.Sleep_info1);
  printField("[CONFIG ] ", "Sleep_info2      = ", config.Sleep_info2);

  Serial.println("[CONFIG ] Wi-Fi");
  printField("[CONFIG ] ", "  phone_ssid     = ", config.phone_ssid[0] ? config.phone_ssid : "(not set)");
  printField("[CONFIG ] ", "  phone_pass     = ", config.phone_pass[0] ? "***" : "(not set)");

  Serial.println();
  Serial.println("[SYSTEM ] ===== System info =====");

  printField("[SYSTEM ] ", "gnss_module      = ", systemInfo.gnss_module);
  printField("[SYSTEM ] ", "gnss_mode        = ", systemInfo.gnss_mode);
  printField("[SYSTEM ] ", "dynamic_model    = ", systemInfo.dynamic_model);
  printField("[SYSTEM ] ", "sample_rate      = ", systemInfo.sample_rate);
  printField("[SYSTEM ] ", "speed_units      = ", systemInfo.speed_units);
  printField("[SYSTEM ] ", "storage_mb       = ", storage_sd_total_mb());
  printField("[SYSTEM ] ", "storage_used_mb  = ", storage_sd_used_mb());
  printField("[SYSTEM ] ", "storage_free_mb  = ", storage_sd_free_mb());
  printField("[SYSTEM ] ", "display          = ", systemInfo.display);
  printField("[SYSTEM ] ", "cpu_freq         = ", getCpuFrequencyMhz());
  printField("[SYSTEM ] ", "software_version = ", systemInfo.software_version);

  Serial.println("=======================================");
  Serial.println();
}
