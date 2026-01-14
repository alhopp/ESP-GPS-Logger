#include "system_info.h"

#include "system_info.h"

const SystemInfo systemInfo = {
  "u-blox NEO-M10",                // gnss_module
  "GPS + GLONASS + GALILEO",       // gnss_mode
  "SEA",                           // dynamic_model
  5,                               // sample_rate

  128,                             // storage_mb

  "Version 1",                     // software_version
  "LilyGO T5 B74",                 // display
  80,                              // cpu_freq

  "Knots",                         // speed_units
  3.6f                             // cal_speed
};
