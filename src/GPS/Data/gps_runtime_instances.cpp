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

GPS_speed M100 (100);
GPS_speed M250 (250);
GPS_speed M500 (500);
GPS_speed M1852(1852);    // 1 nautical mile

GPS_time S2    (2);
GPS_time s2    (2);
GPS_time S10   (10);
GPS_time s10   (10);      // Stats / GPIO12 screens (resettable)
GPS_time S1800 (1800);    // 30-minute window
GPS_time S3600 (3600);    // 60-minute window

Alfa_speed A250(50);
Alfa_speed A500(50);
Alfa_speed a500(50);      // Stats / GPIO12 screens (resettable)
