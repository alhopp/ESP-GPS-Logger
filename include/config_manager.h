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


  // GPS / speed

  int   stat_speed;         // max speed (m/s) to show stat screens
  int   start_logging_speed;

  // Screen / UI configuration  $$$$$$$$$$$$$MOST OF THIS CAN GO
  int   field;              // default speed screen field
  int   Stat_screens;       // enabled stat screens



  // Performance screens (on/off)
  bool stat_2s;
  bool stat_10s;
  bool stat_alpha;
  bool stat_nm;
  bool stat_1h;
  bool stat_distance;



  char  stat_screen[22];    // selected stat screens




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


  // Reference points / geometry
  double p1_lon, p1_lat;
  double p2_lon, p2_lat;
  double p3_lon, p3_lat;
  double p4_lon, p4_lat;

  int track_distance;
};

// Global instance (declaring the global config object)
extern Config config;  



