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

  int   start_logging_speed;

  // Screen / UI configuration  $$$$$$$$$$$$$MOST OF THIS CAN GO
  int   field;              // default speed screen field


  // Performance screens (on/off)
  bool stat_2s;
  bool stat_10s;
  bool stat_alpha;
  bool stat_nm;
  bool stat_1h;
  bool stat_distance;

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
  char home_ssid[32]  = "";
  char home_pass[64]  = "";
  char phone_ssid[32] = "";
  char phone_pass[64] = "";

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



