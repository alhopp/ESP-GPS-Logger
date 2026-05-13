#pragma once

// ============================================================================
// Build feature flags
//
// Compile-time switches supplied by platformio.ini. Defaults here keep local
// builds working when a flag is not provided by the environment.
// ============================================================================

#ifndef GPS_SIMULATOR
#define GPS_SIMULATOR 0
#endif

#ifndef DEV_FORCE_WIFI
#define DEV_FORCE_WIFI 0
#endif

#ifndef LOG_ENABLED
#define LOG_ENABLED 1
#endif

#ifndef STATS_ONLY_SERIAL
#define STATS_ONLY_SERIAL GPS_SIMULATOR
#endif

#ifndef RUN_DETECTOR_DEBUG
#define RUN_DETECTOR_DEBUG 0
#endif

#ifndef SBP_STAT_SAMPLE_DEBUG
#define SBP_STAT_SAMPLE_DEBUG 0
#endif

inline constexpr bool build_gps_simulator_enabled()
{
  return GPS_SIMULATOR != 0;
}

inline constexpr bool build_dev_wifi_enabled()
{
  return DEV_FORCE_WIFI != 0;
}
