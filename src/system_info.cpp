#include "system_info.h"

const SystemInfo systemInfo = {
  .gnss_module          = "u-blox NEO-M10",
  .gnss_mode            = "GPS + GLONASS + GALILEO",
  .dynamic_model        = "SEA",  
  .sample_rate          = 5,

  .storage_mb           = 128,
  
  .software_version     = "Version 1",

  .display              = "LilyGO T5 B74",
  .cpu_freq             = 80,

  .speed_units          = "Knots",
  .cal_speed            = 3.6f,

};
