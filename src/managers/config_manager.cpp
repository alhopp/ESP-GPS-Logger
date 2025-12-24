// -----------------------------------------------------------------------------
// Configuration:
// - Loads configuration from LittleFS (config.txt)
// - Creates and saves defaults if missing or invalid
// - Applies derived runtime values (RTC, calibration, UI settings)
//
// Must run after storage init and before Wi-Fi, logging, or tasks.
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "Definitions.h"        // <-- logging macros
#include "config_manager.h"
#include "ESP_functions.h"
#include "rtc_state.h"

// ----------------------------------------------------
// CONFIG LOCATION (LittleFS ONLY)
// ----------------------------------------------------
#define CONFIG_FILE "/config.txt"

// ----------------------------------------------------
// External state
// ----------------------------------------------------
extern Config config;

// ----------------------------------------------------
// Forward declarations
// ----------------------------------------------------
static void setDefaultConfig();
static bool loadConfigFromFile(File &file);
static void applyDerivedConfig();
static void writeConfigToFile(File &file);

// ----------------------------------------------------
// Public API
// ----------------------------------------------------
void initConfig()
{
  LOG_CONFIG("Init", "Loading configuration");

  // LittleFS MUST be available
  if (!LittleFS.begin(true)) {
    LOG_ERROR("CONFIG", "LittleFS not mounted");
    return;
  }

  // --------------------------------------------------
  // Create default config if missing
  // --------------------------------------------------
  if (!LittleFS.exists(CONFIG_FILE)) {
    LOG_CONFIG("Config", "No config found → creating default");

    setDefaultConfig();

    File f = LittleFS.open(CONFIG_FILE, FILE_WRITE);
    if (!f) {
      LOG_ERROR("CONFIG", "Cannot create config.txt");
      return;
    }

    writeConfigToFile(f);
    f.close();

    applyDerivedConfig();
    return;
  }

  // --------------------------------------------------
  // Load existing config
  // --------------------------------------------------
  File f = LittleFS.open(CONFIG_FILE, FILE_READ);
  if (!f) {
    LOG_ERROR("CONFIG", "Failed to open config.txt");
    setDefaultConfig();
    applyDerivedConfig();
    return;
  }

  if (!loadConfigFromFile(f)) {
    LOG_ERROR("CONFIG", "Invalid config → reset defaults");
    f.close();

    setDefaultConfig();
    File fw = LittleFS.open(CONFIG_FILE, FILE_WRITE);
    if (fw) {
      writeConfigToFile(fw);
      fw.close();
    }
  } else {
    f.close();
  }

  applyDerivedConfig();
  LOG_CONFIG("Init", "Configuration loaded");
}

// ----------------------------------------------------
// Save config (called from Wi-Fi / UI)
// ----------------------------------------------------
void saveConfig()
{
  File f = LittleFS.open(CONFIG_FILE, FILE_WRITE);
  if (!f) {
    LOG_ERROR("CONFIG", "Cannot save config");
    return;
  }

  writeConfigToFile(f);
  f.close();

  LOG_CONFIG("Save", "Configuration saved");
}

// ----------------------------------------------------
// Internal helpers
// ----------------------------------------------------
static void setDefaultConfig()
{
  LOG_CONFIG("Defaults", "Applying defaults");

  config.cal_bat              = 1.75f;
  config.shutdown_voltage     = 3.2f;
  config.cal_speed            = 3.6f;
  config.sample_rate          = 5;
  config.cpu_freq             = 80;
  config.gnss                 = 2;
  config.field                = 1;
  config.speed_large_font     = 0;
  config.bar_length           = 1852;
  config.Stat_screens         = 12;
  config.Stat_screens_time    = 4;
  config.stat_speed           = 1;
  config.start_logging_speed  = 1;
  config.archive_days         = 10;
  config.Board_Logo           = 1;
  config.Sail_Logo            = 1;
  config.sleep_off_screen     = 11;
  config.bat_choice           = 0;
  config.logTXT               = 1;
  config.logUBX               = 0;
  config.logSBP               = 0;
  config.logGPY               = 1;
  config.logGPX               = 0;
  config.file_date_time       = 1;
  config.dynamic_model        = 0;
  config.timezone             = 1.0f;
  config.timezone_DST         = 1;
  config.track_distance       = 1852;

  strlcpy(config.speed_screen,  "1",        sizeof(config.speed_screen));
  strlcpy(config.stat_screen,   "12",       sizeof(config.stat_screen));
  strlcpy(config.gpio12_screen, "4",        sizeof(config.gpio12_screen));
  strlcpy(config.UBXfile,       "/ubxGPS",  sizeof(config.UBXfile));
  strlcpy(config.Sleep_info,    "ESP32 GPS",sizeof(config.Sleep_info));
  strlcpy(config.ssid,          "",         sizeof(config.ssid));
  strlcpy(config.password,      "",         sizeof(config.password));
  strlcpy(config.ssid2,         "ESP32_GPS", sizeof(config.ssid2));
  strlcpy(config.password2,     "",         sizeof(config.password2));
}

// ----------------------------------------------------
static bool loadConfigFromFile(File &file)
{
  StaticJsonDocument<1536> doc;
  DeserializationError err = deserializeJson(doc, file);
  if (err) {
    LOG_ERROR("CONFIG", "JSON parse failed");
    return false;
  }

  // (unchanged field loading)
  config.cal_bat          = doc["cal_bat"] | config.cal_bat;
  config.shutdown_voltage = doc["shutdown_voltage"] | config.shutdown_voltage;
  // ... rest unchanged ...

  return true;
}

// ----------------------------------------------------
static void writeConfigToFile(File &file)
{
  StaticJsonDocument<1536> doc;
  // (unchanged JSON population)
  serializeJsonPretty(doc, file);
}

// ----------------------------------------------------
static void applyDerivedConfig()
{
  LOG_CONFIG("Apply", "Derived runtime values");

  RTC_minimum_voltage_bat = config.shutdown_voltage;
  strcpy(RTC_Sleep_txt, config.Sleep_info);

  RTC_Board_Logo = config.Board_Logo;
  RTC_Sail_Logo  = config.Sail_Logo;

  config.cal_bat = RTC_calibration_bat;
  calibration_speed = config.cal_speed / 1000.0f;

  RTC_SLEEP_screen = config.sleep_off_screen % 10;
  RTC_OFF_screen   = (config.sleep_off_screen / 10) % 10;

  if (config.file_date_time == 0) config.logTXT = 1;

  config.screen_count = strlen(config.stat_screen) - 1;
  config.speed_count  = strlen(config.speed_screen) - 1;
  config.gpio12_count = strlen(config.gpio12_screen) - 1;

  config.field_actual = config.speed_screen[0];
  TimeZone_env(config.timezone);
}
