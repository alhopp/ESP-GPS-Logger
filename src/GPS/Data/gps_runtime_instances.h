#pragma once

// ============================================================================
// gps_runtime_instances.h
//
// Shared GPS runtime objects for the firmware.
//
// These are deliberately global because the original RP6 metric pipeline is
// built around long-lived calculators that read from the shared GPS rings.  The
// definitions live in gps_runtime_instances.cpp so each object is constructed
// exactly once.
// ============================================================================

#include "GPS/Data/gps_data.h"
#include "GPS/Data/gps_satellite_quality.h"

#include "GPS/Metrics/gps_alpha_speed.h"
#include "GPS/Metrics/gps_distance_speed.h"
#include "GPS/Metrics/gps_time_speed.h"

// Raw GPS sample/session state.
extern GPS_data     Ublox;
extern GPS_SAT_info Ublox_Sat;

// Distance-window speed calculators.
extern GPS_distance_speed speed_100m;
extern GPS_distance_speed speed_250m;
extern GPS_distance_speed speed_500m;
extern GPS_distance_speed speed_nm;

// Time-window speed calculators.
extern GPS_time_speed speed_2s;
extern GPS_time_speed speed_10s;
extern GPS_time_speed speed_30min;
extern GPS_time_speed speed_1h;

// Alpha calculators. The current display/storage path uses alpha_500m, while
// alpha_250m is kept because the RP6 calculation model updates both windows.
extern Alfa_speed alpha_250m;
extern Alfa_speed alpha_500m;
