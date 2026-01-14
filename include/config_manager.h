#pragma once

#include <stdint.h>

// ============================================================================
// Configuration Manager API
// ============================================================================

void initConfig();
void ensureConfigExistsOnSD();
void saveConfig();
void TimeZone_env(float timezone);

void loadConfiguration(const char *filename, const char *filename_backup, struct Config &config);

// ============================================================================
// Configuration Structure
// ============================================================================

struct Config
{
  // --------------------------------------------------------------------------
  // Battery / power
  // --------------------------------------------------------------------------
  float cal_bat;            // calibration factor for battery voltage
  float shutdown_voltage;   // shutdown threshold (V)
  bool  bat_choice;         // true = %, false = voltage

  // GPS / speed
  float cal_speed;          // m/s → km/h (knots: 1.944)
  int   sample_rate;        // GPS rate (Hz): 1 / 5 / 10
  int   gnss;               // GNSS mode (GPS + GLONASS default)
  int   dynamic_model;      // 0 = portable, 1 = sea
  int   stat_speed;         // max speed (m/s) to show stat screens
  int   start_logging_speed;

  // Screen / UI configuration
  int   field;              // default speed screen field
  int   field_actual;       // current field
  int   speed_large_font;   // large font on first line
  int   Stat_screens;       // enabled stat screens
  int   Stat_screens_time;  // seconds per stat screen
  int   GPIO12_screens;     // stat screens when GPIO12 active
  int   sleep_off_screen;
  int   Board_Logo;
  int   Sail_Logo;

  char  stat_screen[22];    // selected stat screens
  char  gpio12_screen[10];  // screens when GPIO12 toggles
  char  speed_screen[10];   // selected speed fields

  int   screen_count;
  int   gpio12_count;
  int   speed_count;

  // Time / localisation
  float timezone;           // UTC offset (hours)
  bool  timezone_DST;       // automatic daylight saving

  // Logging
  bool  logTXT;
  bool  logUBX;
  bool  logUBX_nav_sat;
  bool  logSBP;
 
  int   file_date_time;
  int   archive_days;
  int   bar_length;

  // Identification / filenames
  char  UBXfile[32];
  char  Sleep_info[32];

  // Wi-Fi credentials
  char  ssid[32];
  char  password[32];
  char  ssid2[32];
  char  password2[32];

  // System / diagnostics
  int     config_fail;
  uint8_t ublox_type;
  int     cpu_freq;

  // Reference points / geometry
  double p1_lon, p1_lat;
  double p2_lon, p2_lat;
  double p3_lon, p3_lat;
  double p4_lon, p4_lat;

  int track_distance;
};

// Global instance (declaring the global config object)
extern Config config;  



