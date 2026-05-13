#include "Core/system_info.h"

// ============================================================================
// System information
//
// Central definition of the static device metadata declared in system_info.h.
// ============================================================================

#include "build_version.h"

const SystemInfo systemInfo = {
  "u-blox NEO-M10",                // gnss_module
  "GPS + GLONASS + GALILEO",       // gnss_mode
  "SEA",                           // dynamic_model
  5,                               // sample_rate

  GPS_SOFTWARE_VERSION,              // software_version
  "LilyGO T5 B74",                 // display

  "Knots",                         // speed_units
  3.6f                             // cal_speed
};
