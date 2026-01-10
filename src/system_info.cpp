#include "system_info.h"

const SystemInfo systemInfo = {
  .gnss_module     = "u-blox NEO-M10",
  .storage_mb      = 128,
  .software_version= "Version 1",

  .speed_units     = "Knots",
  .sample_rate     = "5 Hz",
  .gnss_mode       = "GPS + GLONASS + GALILEO",
  .dynamic_model   = "SEA",
  .display         = "LilyGO T5 B74"

};
