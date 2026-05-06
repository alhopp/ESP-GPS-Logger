#pragma once

#ifndef GPS_SIMULATOR
#define GPS_SIMULATOR 0
#endif

#ifndef DEV_FORCE_WIFI
#define DEV_FORCE_WIFI 0
#endif

#ifndef LOG_ENABLED
#define LOG_ENABLED 1
#endif

inline constexpr bool build_gps_simulator_enabled()
{
  return GPS_SIMULATOR != 0;
}

inline constexpr bool build_dev_wifi_enabled()
{
  return DEV_FORCE_WIFI != 0;
}
