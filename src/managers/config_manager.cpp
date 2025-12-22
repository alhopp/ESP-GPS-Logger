#include <Arduino.h>
#include "config_manager.h"
#include "SD_card.h"
#include "ESP_functions.h"

// ----------------------------------------------------
// External state required for configuration loading
// ----------------------------------------------------
extern bool sdOK;
extern bool LITTLEFS_OK;

extern Config      config;
extern const char* filename;
extern const char* filename_backup;

// ----------------------------------------------------
// Public API
// ----------------------------------------------------
void initConfig()
{
  // Configuration requires persistent storage
  if (!(sdOK || LITTLEFS_OK)) {
    Serial.println(F("No storage available — skipping config load"));
    return;
  }

  Serial.println(F("[CONFIG ] Loading configuration..."));

  ensureConfigExistsOnSD();
  loadConfiguration(filename, filename_backup, config);

  printFile(filename);
}


void ensureConfigExistsOnSD() {
  if (!SD_MMC.exists("/config.txt")) {
    Serial.println("config.txt missing on SD → creating default");

    File f = SD_MMC.open("/config.txt", FILE_WRITE);
    if (!f) {
      Serial.println("Failed to create /config.txt on SD");
      return;
    }

    StaticJsonDocument<512> doc;
    doc["cal_bat"] = 1.75;
    doc["cal_speed"] = 3.6;
    doc["sample_rate"] = 5;
    doc["gnss"] = 2;
    doc["ssid"] = "ssid_not_set";
    doc["password"] = "password";

    serializeJsonPretty(doc, f);
    f.close();

    Serial.println("Default config.txt written to SD");
  }
}


void loadConfiguration(const char *filename,
                       const char *filename_backup,
                       Config &config)
{
  File file;

  // --------------------------------------------------
  // 1. Open config file (SD first, then LittleFS)
  // --------------------------------------------------
  auto openFromFS = [&](fs::FS &fs) -> bool {
    if (fs.exists(filename)) {
      Serial.println(F("[CONFIG] open config.txt"));
      file = fs.open(filename, FILE_READ);
      return true;
    }
    if (fs.exists(filename_backup)) {
      Serial.println(F("[CONFIG ] open config_backup.txt"));
      file = fs.open(filename_backup, FILE_READ);
      return true;
    }
    return false;
  };

  bool opened = false;

  if (sdOK)        opened = openFromFS(SD_MMC);
  if (!opened && LITTLEFS_OK) opened = openFromFS(LITTLEFS);

  if (!opened || !file) {
    Serial.println(F("[CONFIG] No configuration file found"));
    config.config_fail = 1;
    return;
  }

  // --------------------------------------------------
  // 2. Parse JSON
  // --------------------------------------------------
  StaticJsonDocument<1536> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.println(F("[CONFIG] Failed to deserialize config"));
    Serial.println(error.f_str());
    config.config_fail = 1;
    return;
  }

  // --------------------------------------------------
  // 3. Load values with defaults
  // --------------------------------------------------
  config.cal_bat            = doc["cal_bat"]            | 1.75f;
  config.shutdown_voltage   = doc["shutdown_voltage"]   | 3.2f;
  config.cal_speed          = doc["cal_speed"]          | 3.6f;
  config.sample_rate        = doc["sample_rate"]        | 5;
  config.cpu_freq           = doc["cpu_freq"]           | 80;
  config.gnss               = doc["gnss"]               | 3;
  config.field              = doc["speed_field"]        | 1;
  config.speed_large_font   = doc["speed_large_font"]   | 0;
  config.bar_length         = doc["bar_length"]         | 1852;
  config.Stat_screens       = doc["Stat_screens"]       | 12;
  config.Stat_screens_time  = doc["Stat_screens_time"]  | 4;
  config.stat_speed         = doc["stat_speed"]         | 1;
  config.start_logging_speed= doc["start_logging_speed"]| 1;
  config.archive_days       = doc["archive_days"]       | 10;
  config.Board_Logo         = doc["Board_Logo"]         | 1;
  config.Sail_Logo          = doc["Sail_Logo"]          | 1;
  config.sleep_off_screen   = doc["sleep_off_screen"]   | 11;
  config.bat_choice         = doc["bat_choice"]         | 0;
  config.logTXT             = doc["logTXT"]             | 1;
  config.logUBX             = doc["logUBX"]             | 0;
  config.logSBP             = doc["logSBP"]             | 0;
  config.logGPY             = doc["logGPY"]             | 1;
  config.logGPX             = doc["logGPX"]             | 0;
  config.file_date_time     = doc["file_date_time"]     | 1;
  config.dynamic_model      = doc["dynamic_model"]      | 0;
  config.timezone           = doc["timezone"]           | 1.0f;
  config.timezone_DST       = doc["timezone_DST"]       | 1;
  config.track_distance     = doc["track_distance"]     | 1852;

  // Conditional flags
  config.logUBX_nav_sat =
    (config.sample_rate < 10) ? (doc["logUBX_nav_sat"] | 0) : 0;

  // Strings
  strlcpy(config.speed_screen, doc["speed_screen"] | "1",  sizeof(config.speed_screen));
  strlcpy(config.stat_screen,  doc["stat_screen"]  | "12", sizeof(config.stat_screen));
  strlcpy(config.gpio12_screen,doc["gpio12_screen"]| "4",  sizeof(config.gpio12_screen));
  strlcpy(config.UBXfile,      doc["UBXfile"]      | "/ubxGPS", sizeof(config.UBXfile));
  strlcpy(config.Sleep_info,   doc["Sleep_info"]   | "My ID",   sizeof(config.Sleep_info));
  strlcpy(config.ssid,         doc["ssid"]         | "ssid_not_set", sizeof(config.ssid));
  strlcpy(config.password,     doc["password"]     | "password",     sizeof(config.password));
  strlcpy(config.ssid2,        doc["ssid2"]        | "ESP_GPS", sizeof(config.ssid2));
  strlcpy(config.password2,    doc["password2"]    | "password2", sizeof(config.password2));


  

  // --------------------------------------------------
  // 4. Post-load fixes & derived values
  // --------------------------------------------------
  RTC_minimum_voltage_bat = config.shutdown_voltage;
  strcpy(RTC_Sleep_txt, config.Sleep_info);

  RTC_Board_Logo = config.Board_Logo;
  RTC_Sail_Logo  = config.Sail_Logo;

  config.cal_bat = RTC_calibration_bat;
  calibration_speed = config.cal_speed / 1000.0f;

  RTC_SLEEP_screen = config.sleep_off_screen % 10;
  RTC_OFF_screen   = (config.sleep_off_screen / 10) % 10;

  if (config.file_date_time == 0) config.logTXT = 1;

  config.screen_count  = strlen(config.stat_screen)  - 1;
  config.speed_count   = strlen(config.speed_screen) - 1;
  config.gpio12_count  = strlen(config.gpio12_screen)- 1;
  config.field_actual  = config.speed_screen[0];

  TimeZone_env(config.timezone);
}

void saveConfig()
{
  if (!(sdOK || LITTLEFS_OK)) {
    Serial.println(F("[CONFIG ] No storage available — cannot save config"));
    return;
  }

  fs::FS* fs = nullptr;

  if (sdOK) {
    fs = &SD_MMC;
  } else if (LITTLEFS_OK) {
    fs = &LITTLEFS;
  }

  if (!fs) {
    Serial.println(F("[CONFIG ] No filesystem available"));
    return;
  }

  Serial.println(F("[CONFIG ] Saving configuration"));

  File f = fs->open("/config.txt", FILE_WRITE);
  if (!f) {
    Serial.println(F("[CONFIG ] Failed to open config.txt for writing"));
    return;
  }

  StaticJsonDocument<1536> doc;

  // ---- mirror loadConfiguration() ----
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

  // Strings
  doc["speed_screen"]   = config.speed_screen;
  doc["stat_screen"]    = config.stat_screen;
  doc["gpio12_screen"]  = config.gpio12_screen;
  doc["UBXfile"]        = config.UBXfile;
  doc["Sleep_info"]     = config.Sleep_info;
  doc["ssid"]           = config.ssid;
  doc["password"]       = config.password;
  doc["ssid2"]          = config.ssid2;
  doc["password2"]      = config.password2;

  serializeJsonPretty(doc, f);
  f.close();

  Serial.println(F("[CONFIG ] Configuration saved"));
}



