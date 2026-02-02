#pragma once
#include <stdint.h>

// ============================================================================
// Run detection
// ============================================================================
// Detects the start of a new run based on heading evolution and short-term speed
int New_run_detection(float actual_heading, float S2_speed);

// ============================================================================
// GPS_Track
//
// Fixed-distance track timing (e.g. 500 m speed run).
// - Two virtual lines define start & end gates
// - Crossing start arms the run
// - Crossing end finalises timing and speed
// ============================================================================
class GPS_Track {
public:
    GPS_Track();

    // Define track geometry and nominal distance (meters)
    void Set_course(double lon_1,double lat_1,
                    double lon_2,double lat_2,
                    double lon_3,double lat_3,
                    double lon_4,double lat_4,
                    int distance);

    // Update track state; returns signed distance to end line
    float Update_Track();

    // ------------------------------------------------------------------------
    // Track geometry (ordered internally)
    // ------------------------------------------------------------------------
    double lon1,lat1,lon2,lat2;
    double lon3,lat3,lon4,lat4;

    // Start / end crossing positions
    double Start_lon,Start_lat;
    double End_lon,End_lat;

    // ------------------------------------------------------------------------
    // Timing & distance
    // ------------------------------------------------------------------------
    int   Start_iTOW_ms;               // UBX time at start crossing
    int   End_iTOW_ms;                 // UBX time at end crossing
    int   Track_time_ms;               // Run duration
    float Track_speed;                 // Average speed
    float track_distance;              // GPS-measured distance
    int   theoretical_track_distance;  // Nominal course length

    // Line distances (signed)
    float distance_startline;
    float distance_endline;
    float distance_p1p3, distance_p2p4;

    // ------------------------------------------------------------------------
    // Result buffers (sorted runs)
    // ------------------------------------------------------------------------
    double  avg_speed[10];
    double  display_speed[10];
    int     m_Distance[10];
    uint8_t time_hour[10], time_min[10], time_sec[10];

    // Legacy / placeholder (can be removed later)
    uint8_t dummy[10];
    int     dummy_int[10];

private:
    float Old_distance_start = 0.0f;   // Previous start-line distance
    float Old_distance_end   = 0.0f;   // Previous end-line distance
    bool  Run_started        = false;  // Run-in-progress flag
};

