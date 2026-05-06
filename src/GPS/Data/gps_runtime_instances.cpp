#include "GPS/Data/gps_data.h"

// Owns the global GPS/statistics objects used by the firmware.
// This preserves the RP6 global-object model while keeping construction out of
// headers and avoiding hidden duplicate instances.

#include "GPS/Data/gps_satellite_quality.h"

#include "GPS/Metrics/gps_alpha_speed.h"
#include "GPS/Metrics/gps_distance_speed.h"
#include "GPS/Metrics/gps_time_speed.h"

// Global GPS runtime instances, constructed once for the firmware lifetime.

GPS_data     Ublox;       // Circular buffers + distance accumulation
GPS_SAT_info Ublox_Sat;   // NAV-SAT signal quality statistics

GPS_distance_speed speed_100m (100);
GPS_distance_speed speed_250m (250);
GPS_distance_speed speed_500m (500);
GPS_distance_speed speed_nm(1852);    // 1 nautical mile

GPS_time_speed speed_2s    (2);
GPS_time_speed speed_10s   (10);
GPS_time_speed speed_30min (1800);    // 30-minute window
GPS_time_speed speed_1h (3600);    // 60-minute window

Alfa_speed alpha_250m(50);
Alfa_speed alpha_500m(50);
