// -----------------------------------------------------------------------------
// Configuration Manager
//
// Loads / validates config from LittleFS (/config.txt), creates defaults if
// missing or invalid, applies derived runtime values, and dumps diagnostics.
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <LittleFS.h>

#include "Core/Definitions.h"
#include "Core/Globals.h"
#include "Core/rtc_state.h"
#include "core/system_info.h"
#include "managers/config_defaults.h"
#include "managers/config_json.h"
#include "managers/config_manager.h"
#include "managers/config_validation.h"

namespace {
constexpr const char* CONFIG_FILE = "/config.txt";

void applyDerivedConfig();
void dumpConfig();
}

Config config;

void initConfig()
{
  LOG_CONFIG("Init", "Loading configuration");

  if (!LittleFS.exists(CONFIG_FILE)) {
    LOG_CONFIG("Config", "No config found, creating default");
    config_set_defaults();

    File f = LittleFS.open(CONFIG_FILE, FILE_WRITE);
    if (!f) {
      LOG_ERROR("CONFIG", "Cannot create config.txt");
      return;
    }

    config_write_json(f);
    f.close();

    config_validate();
    applyDerivedConfig();
    dumpConfig();
    return;
  }

  File f = LittleFS.open(CONFIG_FILE, FILE_READ);
  if (!f) {
    LOG_ERROR("CONFIG", "Failed to open config.txt");
    config_set_defaults();
    config_validate();
    applyDerivedConfig();
    dumpConfig();
    return;
  }

  if (!config_load_json(f)) {
    LOG_ERROR("CONFIG", "Invalid config, reset defaults");
    f.close();

    config_set_defaults();
    File fw = LittleFS.open(CONFIG_FILE, FILE_WRITE);
    if (fw) {
      config_write_json(fw);
      fw.close();
    }
  } else {
    f.close();
  }

  config_validate();
  applyDerivedConfig();

  LOG_CONFIG("Init", "Configuration loaded");
  dumpConfig();
}

void saveConfig()
{
  config_validate();

  File f = LittleFS.open(CONFIG_FILE, FILE_WRITE);
  if (!f) {
    LOG_ERROR("CONFIG", "Cannot save config");
    return;
  }

  config_write_json(f);
  f.close();

  LOG_CONFIG("Save", "Configuration saved, dumping final state");
  dumpConfig();
}

namespace {

void applyDerivedConfig()
{
  LOG_CONFIG("Apply", "Derived runtime values");

  RTC_minimum_voltage_bat = config.shutdown_voltage;
  TimeZone_env(config.timezone);
}

void dumpConfig()
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

} // namespace

void TimeZone_env(float timezone)
{
  int hours = (int)(timezone);
  int minutes = abs((int)(timezone * 60) % 60);

  char time_noDST[64] = "GMT0";

  if (hours > 0) {
    sprintf(time_noDST, "CET-%d:%02d", hours, minutes);
  } else {
    sprintf(time_noDST, "CET+%d:%02d", -hours, minutes);
  }

  strcpy(TimeZone, time_noDST);

  if (config.timezone_DST) {
    switch ((int)(timezone * 100)) {
      case 0:    strcpy(TimeZone, "GMT0BST,M3.5.0/1,M10.5.0"); break;
      case 100:  strcpy(TimeZone, "CET-1CEST,M3.5.0,M10.5.0/3"); break;
      case 200:  strcpy(TimeZone, "EET-2EEST,M3.5.0,M10.5.0/3"); break;
      case 300:  strcpy(TimeZone, "<-03>3<-02>,M3.2.0,M11.1.0"); break;
      case 500:  strcpy(TimeZone, "CST5CDT,M3.2.0/0,M11.1.0/1"); break;
      case 600:  strcpy(TimeZone, "CST6CDT,M3.2.0,M11.1.0"); break;
      case 700:  strcpy(TimeZone, "MST7MDT,M3.2.0,M11.1.0"); break;
      case 800:  strcpy(TimeZone, "PST8PDT,M3.2.0,M11.1.0"); break;
      case 950:  strcpy(TimeZone, "ACST-9:30ACDT,M10.1.0,M4.1.0/3"); break;
      case 1000: strcpy(TimeZone, "AEST-10AEDT,M10.1.0,M4.1.0/3"); break;
      case 1050: strcpy(TimeZone, "<+1030>-10:30<+11>-11,M10.1.0,M4.1.0"); break;
      case 1200: strcpy(TimeZone, "NZST-12NZDT,M9.5.0,M4.1.0/3"); break;
      case -100: strcpy(TimeZone, "<-01>1<+00>,M3.5.0/0,M10.5.0/1"); break;
      case -200: strcpy(TimeZone, "IST-2IDT,M3.4.4/26,M10.5.0"); break;
    }
  }
}
