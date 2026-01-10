#pragma once
#include <stdint.h>

struct SystemInfo {
  const char* gnss_module;     // e.g. "u-blox NEO-M10"
  uint32_t    storage_mb;      // e.g. 128
  const char* software_version;// e.g. "Version 1"

  const char* speed_units;     // "Knots"
  const char* sample_rate;     // "5 Hz"
  const char* gnss_mode;       // "GPS + GLONASS + GALILEO"
  const char* dynamic_model;   // "SEA"
  
  const char* display;         // "LilyGO T5 B74"   
};

extern const SystemInfo systemInfo;
