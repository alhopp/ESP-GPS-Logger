#pragma once

// ============================================================================
// System information
//
// Static device metadata reported through the web API and status/debug output.
// Keep this file to build/device facts; runtime state belongs in the relevant
// module or RTC state file.
// ============================================================================

#include <stdint.h>


struct SystemInfo {
  const char* gnss_module;        // e.g. "u-blox NEO-M10"
  const char* gnss_mode;          // "GPS + GLONASS + GALILEO"
  const char* dynamic_model;      // "SEA"
  uint32_t    sample_rate;        // "5"

  const char* software_version;   // e.g. "Version 1"

  const char* display;            // "LilyGO T5 B74"   

  const char* speed_units;        // "Knots"

};
extern const SystemInfo systemInfo;
