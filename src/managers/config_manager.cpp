#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

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

// RTC / derived globals already used elsewhere
extern float RTC_calibration_bat;
extern float calibration_speed;
extern float RTC_minimum_voltage_bat;
extern char  RTC_Sleep_txt[32];
extern int   RTC_Board_Logo;
extern int   RTC_Sail_Logo;
extern int   RTC_SLEEP_screen;
extern int   RTC_OFF_screen;

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
  Serial.println("[CONFIG ] Loading configuration...");

  // LittleFS MUST be available
  if (!LittleFS.begin(true)) {
    Serial.println("[CONFIG ] FATAL: LittleFS not mounted");
    return;
  }

  // --------------------------------------------------
  // Create default config if missing
  // --------------------------------------------------
  if (!LittleFS.exists(CONFIG_FILE)) {
    Serial.println("[CONFIG ] No config found → creating default");

    setDefaultConfig();

    File f = LittleFS.open(CONFIG_FILE, FILE_WRITE);
    if (!f) {
      Serial.println("[CONFIG ] ERROR: Cannot create config.txt");
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
    Serial.println("[CONFIG ] ERROR: Failed to open config.txt");
    setDefaultConfig();
    applyDerivedConfig();
    return;
  }

  if (!loadConfigFromFile(f)) {
    Serial.println("[CONFIG ] ERROR: Invalid config → reset defaults");
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
  Serial.println("[CONFIG ] Configuration loaded");
}

// ----------------------------------------------------
// Save config (called from Wi-Fi / UI)
// ----------------------------------------------------
void saveConfig()
{
  File f = LittleFS.open(CONFIG_FILE, FILE_WRITE);
  if (!f) {
    Serial.println("[CONFIG ] ERROR: Cannot save config");
    return;
  }

  writeConfigToFile(f);
  f.close();

  Serial.println("[CONFIG ] Configuration saved");
}

// ----------------------------------------------------
// Internal helpers
// ----------------------------------------------------
static void setDefaultConfig()
{
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
  if (err) return false;

  config.cal_bat             = doc["cal_bat"]             | config.cal_bat;
  config.shutdown_voltage    = doc["shutdown_voltage"]    | config.shutdown_voltage;
  config.cal_speed           = doc["cal_speed"]           | config.cal_speed;
  config.sample_rate         = doc["sample_rate"]         | config.sample_rate;
  config.cpu_freq            = doc["cpu_freq"]            | config.cpu_freq;
  config.gnss                = doc["gnss"]                | config.gnss;
  config.field               = doc["speed_field"]         | config.field;
  config.speed_large_font    = doc["speed_large_font"]    | config.speed_large_font;
  config.bar_length          = doc["bar_length"]          | config.bar_length;
  config.Stat_screens        = doc["Stat_screens"]        | config.Stat_screens;
  config.Stat_screens_time   = doc["Stat_screens_time"]   | config.Stat_screens_time;
  config.stat_speed          = doc["stat_speed"]          | config.stat_speed;
  config.start_logging_speed = doc["start_logging_speed"] | config.start_logging_speed;
  config.archive_days        = doc["archive_days"]        | config.archive_days;
  config.Board_Logo          = doc["Board_Logo"]          | config.Board_Logo;
  config.Sail_Logo           = doc["Sail_Logo"]           | config.Sail_Logo;
  config.sleep_off_screen    = doc["sleep_off_screen"]    | config.sleep_off_screen;
  config.bat_choice          = doc["bat_choice"]          | config.bat_choice;
  config.logTXT              = doc["logTXT"]              | config.logTXT;
  config.logUBX              = doc["logUBX"]              | config.logUBX;
  config.logSBP              = doc["logSBP"]              | config.logSBP;
  config.logGPY              = doc["logGPY"]              | config.logGPY;
  config.logGPX              = doc["logGPX"]              | config.logGPX;
  config.file_date_time      = doc["file_date_time"]      | config.file_date_time;
  config.dynamic_model       = doc["dynamic_model"]       | config.dynamic_model;
  config.timezone            = doc["timezone"]            | config.timezone;
  config.timezone_DST        = doc["timezone_DST"]        | config.timezone_DST;
  config.track_distance      = doc["track_distance"]      | config.track_distance;

  strlcpy(config.speed_screen,  doc["speed_screen"]  | config.speed_screen,  sizeof(config.speed_screen));
  strlcpy(config.stat_screen,   doc["stat_screen"]   | config.stat_screen,   sizeof(config.stat_screen));
  strlcpy(config.gpio12_screen, doc["gpio12_screen"] | config.gpio12_screen, sizeof(config.gpio12_screen));
  strlcpy(config.UBXfile,       doc["UBXfile"]       | config.UBXfile,       sizeof(config.UBXfile));
  strlcpy(config.Sleep_info,    doc["Sleep_info"]    | config.Sleep_info,    sizeof(config.Sleep_info));
  strlcpy(config.ssid,          doc["ssid"]          | config.ssid,          sizeof(config.ssid));
  strlcpy(config.password,      doc["password"]      | config.password,      sizeof(config.password));
  strlcpy(config.ssid2,         doc["ssid2"]         | config.ssid2,         sizeof(config.ssid2));
  strlcpy(config.password2,     doc["password2"]     | config.password2,     sizeof(config.password2));

  return true;
}

// ----------------------------------------------------
static void writeConfigToFile(File &file)
{
  StaticJsonDocument<1536> doc;

  doc["cal_bat"]              = config.cal_bat;
  doc["shutdown_voltage"]     = config.shutdown_voltage;
  doc["cal_speed"]            = config.cal_speed;
  doc["sample_rate"]          = config.sample_rate;
  doc["cpu_freq"]             = config.cpu_freq;
  doc["gnss"]                 = config.gnss;
  doc["speed_field"]          = config.field;
  doc["speed_large_font"]     = config.speed_large_font;
  doc["bar_length"]           = config.bar_length;
  doc["Stat_screens"]         = config.Stat_screens;
  doc["Stat_screens_time"]    = config.Stat_screens_time;
  doc["stat_speed"]           = config.stat_speed;
  doc["start_logging_speed"]  = config.start_logging_speed;
  doc["archive_days"]         = config.archive_days;
  doc["Board_Logo"]           = config.Board_Logo;
  doc["Sail_Logo"]            = config.Sail_Logo;
  doc["sleep_off_screen"]     = config.sleep_off_screen;
  doc["bat_choice"]           = config.bat_choice;
  doc["logTXT"]               = config.logTXT;
  doc["logUBX"]               = config.logUBX;
  doc["logSBP"]               = config.logSBP;
  doc["logGPY"]               = config.logGPY;
  doc["logGPX"]               = config.logGPX;
  doc["file_date_time"]       = config.file_date_time;
  doc["dynamic_model"]        = config.dynamic_model;
  doc["timezone"]             = config.timezone;
  doc["timezone_DST"]         = config.timezone_DST;
  doc["track_distance"]       = config.track_distance;

  doc["speed_screen"]  = config.speed_screen;
  doc["stat_screen"]   = config.stat_screen;
  doc["gpio12_screen"] = config.gpio12_screen;
  doc["UBXfile"]       = config.UBXfile;
  doc["Sleep_info"]    = config.Sleep_info;
  doc["ssid"]          = config.ssid;
  doc["password"]      = config.password;
  doc["ssid2"]         = config.ssid2;
  doc["password2"]     = config.password2;

  serializeJsonPretty(doc, file);
}

// ----------------------------------------------------
static void applyDerivedConfig()
{
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
