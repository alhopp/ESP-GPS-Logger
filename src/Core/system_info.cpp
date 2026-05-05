#include "Core/system_info.h"

const SystemInfo systemInfo = {
  "u-blox NEO-M10",                // gnss_module
  "GPS + GLONASS + GALILEO",       // gnss_mode
  "SEA",                           // dynamic_model
  5,                               // sample_rate

  128,                             // storage_mb

  "2026.02.14",                     // software_version
  "LilyGO T5 B74",                 // display
  80,                              // cpu_freq

  "Knots",                         // speed_units
  3.6f                             // cal_speed
};
