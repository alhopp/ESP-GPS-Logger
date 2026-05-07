#include "GPS/Data/gps_runtime_instances.h"

// ============================================================================
// GPS runtime object definitions
//
// This file is the single owner of the shared GPS data and metric calculators.
// Keeping construction here avoids duplicate global instances and makes the
// firmware's metric pipeline easy to find:
//
//   NAV-PVT sample -> Ublox rings -> metric calculators -> display/storage
// ============================================================================

// Raw sample rings and satellite-quality aggregation.
GPS_data     Ublox;
GPS_SAT_info Ublox_Sat;

// Distance-window speed calculators. Constructor argument is the target window
// distance in meters.
GPS_distance_speed speed_100m(100);
GPS_distance_speed speed_250m(250);
GPS_distance_speed speed_500m(500);
GPS_distance_speed speed_nm(1852);      // 1 nautical mile.

// Time-window speed calculators. Constructor argument is the window length in
// seconds.
GPS_time_speed speed_2s(2);
GPS_time_speed speed_10s(10);
GPS_time_speed speed_1h(3600);          // 60 minutes.

// Alpha calculators. Constructor argument is the closure radius in meters.
Alfa_speed alpha_500m(50);
