#pragma once
#include <stdint.h>


struct SystemInfo {
  const char* gnss_module;        // e.g. "u-blox NEO-M10"
  const char* gnss_mode;          // "GPS + GLONASS + GALILEO"
  const char* dynamic_model;      // "SEA"
  uint32_t    sample_rate;        // "5"

  uint32_t    storage_mb;         // e.g. 128

  const char* software_version;   // e.g. "Version 1"
  
  const char* display;            // "LilyGO T5 B74"   
  uint32_t    cpu_freq;           // e.g. 80 MHz

  const char* speed_units;        // "Knots"
  float       cal_speed;          // e.g. 3.6

};
extern const SystemInfo systemInfo;
