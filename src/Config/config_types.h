#pragma once

#include <stdint.h>

struct Config
{
  // Battery / power
  float cal_bat;
  float shutdown_voltage;

  // GPS / speed
  int start_logging_speed;

  // Screen / UI configuration
  int field;

  // Performance screens
  bool stat_2s;
  bool stat_10s;
  bool stat_alpha;
  bool stat_nm;
  bool stat_1h;
  bool stat_distance;

  int gpio12_count;
  int speed_count;

  // Time / localisation
  float timezone;
  bool timezone_DST;

  // Logging
  bool logTXT;
  bool logUBX;
  bool logUBX_nav_sat;
  bool logSBP;

  int archive_days;
  int bar_length;

  // Identification / filenames
  char UBXfile[32];
  char Sleep_info1[32];
  char Sleep_info2[32];

  // Wi-Fi credentials
  char home_ssid[32] = "";
  char home_pass[64] = "";
  char phone_ssid[32] = "";
  char phone_pass[64] = "";

  // System / diagnostics
  int config_fail;
  uint8_t ublox_type;

  // Reference points / geometry
  double p1_lon;
  double p1_lat;
  double p2_lon;
  double p2_lat;
  double p3_lon;
  double p3_lat;
  double p4_lon;
  double p4_lat;

  int track_distance;
};

extern Config config;

