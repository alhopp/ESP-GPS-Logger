#pragma once

#include <stdint.h>

// ============================================================================
// Configuration Manager API
// ============================================================================

void initConfig();
void ensureConfigExistsOnSD();
void saveConfig();
void TimeZone_env(float timezone);

void loadConfiguration(const char *filename,
                       const char *filename_backup,
                       struct Config &config);

// ============================================================================
// Configuration Structure
//
// NOTE:
// - This struct is the single source of truth for device configuration
// - It is persisted to SD / LittleFS
// - It is safe to expose via Web API (read/write)
// ============================================================================

struct Config
{
  // --------------------------------------------------------------------------
  // Battery / power
  // --------------------------------------------------------------------------
  float cal_bat           = 1.74f;   // calibration factor for battery voltage
  float shutdown_voltage  = 3.2f;    // shutdown threshold (V)
  bool  bat_choice        = true;    // true = %, false = voltage

  // --------------------------------------------------------------------------
  // GPS / speed
  // --------------------------------------------------------------------------
  float cal_speed         = 3.6f;    // m/s → km/h (knots: 1.944)
  int   sample_rate       = 5;       // GPS rate (Hz): 1 / 5 / 10
  int   gnss              = 3;       // GNSS mode (GPS + GLONASS default)
  int   dynamic_model     = 0;       // 0 = portable, 1 = sea
  int   stat_speed        = 1;       // max speed (m/s) to show stat screens
  int   start_logging_speed = 1;

  // --------------------------------------------------------------------------
  // Screen / UI configuration
  // --------------------------------------------------------------------------
  int   field              = 1;      // default speed screen field
  int   field_actual       = 1;      // current field
  int   speed_large_font   = 1;      // large font on first line
  int   Stat_screens       = 123;    // enabled stat screens
  int   Stat_screens_time  = 4;      // seconds per stat screen
  int   GPIO12_screens     = 54;     // stat screens when GPIO12 active
  int   sleep_off_screen   = 11;
  int   Board_Logo         = 1;
  int   Sail_Logo          = 1;

  char  stat_screen[22]    = "167";  // selected stat screens
  char  gpio12_screen[10];           // screens when GPIO12 toggles
  char  speed_screen[10];            // selected speed fields

  int   screen_count       = 0;
  int   gpio12_count       = 0;
  int   speed_count        = 0;

  // --------------------------------------------------------------------------
  // Time / localisation
  // --------------------------------------------------------------------------
  float timezone           = 1.0f;   // UTC offset (hours)
  bool  timezone_DST       = true;   // automatic daylight saving

  // --------------------------------------------------------------------------
  // Logging
  // --------------------------------------------------------------------------
  bool  logTXT             = true;
  bool  logUBX             = true;
  bool  logUBX_nav_sat     = false;
  bool  logSBP             = true;
  bool  logGPY             = true;
  bool  logGPX             = false;

  int   file_date_time     = 2;      // filename style (MAC / datetime)
  int   archive_days       = 10;     // days before moving to Archive
  int   bar_length         = 1852;   // run length indicator (m, nautical mile)

  // --------------------------------------------------------------------------
  // Identification / filenames
  // --------------------------------------------------------------------------
  char  UBXfile[32]        = "My_ESP_GPS";
  char  Sleep_info[32]     = "Your ID";

  // --------------------------------------------------------------------------
  // Wi-Fi credentials
  // --------------------------------------------------------------------------
  char  ssid[32]           = "My_SSID";
  char  password[32]       = "password";
  char  ssid2[32]          = "ESP_GPS";
  char  password2[32]      = "password2";

  // --------------------------------------------------------------------------
  // System / diagnostics
  // --------------------------------------------------------------------------
  int     config_fail      = 0;
  uint8_t ublox_type       = 0;
  uint8_t M10_high_nav     = 0;
  int     cpu_freq         = 80;

  // --------------------------------------------------------------------------
  // Reference points / geometry
  // --------------------------------------------------------------------------
  double p1_lon, p1_lat;
  double p2_lon, p2_lat;
  double p3_lon, p3_lat;
  double p4_lon, p4_lat;

  int track_distance;
};

// ============================================================================
// Global instance
// ============================================================================

extern Config config;
