#pragma once

// ============================================================================
// Config data model
//
// In-memory representation of /config.txt. Field names intentionally mirror the
// persisted JSON keys and web API groups, so keep additions compatible and do
// not rename existing fields without a migration.
// ============================================================================

#include <stdint.h>

struct Config
{
  // Battery / power
  float cal_bat;
  float shutdown_voltage;

  // Performance screens
  bool stat_2s;
  bool stat_10s;
  bool stat_alpha;
  bool stat_nm;
  bool stat_1h;
  bool stat_distance;

  // Time / localisation
  float timezone;
  bool timezone_DST;

  // Logging
  bool logUBX;
  bool logSBP;

  // Identification / filenames
  char Sleep_info1[32];
  char Sleep_info2[32];

  // Wi-Fi credentials
  char phone_ssid[32] = "";
  char phone_pass[64] = "";
};

extern Config config;
