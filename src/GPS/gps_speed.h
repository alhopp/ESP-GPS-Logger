#pragma once
#include <stdint.h>

// -----------------------------------------------------------------------------
// GPS_speed
// Distance-based average speed calculator
//
// Used for:
//   - 100 m, 250 m, 500 m, 1852 m (1 NM)
//   - Best run detection
//   - Alpha speed support (via m_speed_alfa)
//
// Operates on global GPS ring buffers (_gSpeed / index_GPS)
// -----------------------------------------------------------------------------
class GPS_speed {
public:
    // afstand = distance window in meters (e.g. 100 / 250 / 500 / 1852)
    explicit GPS_speed(int afstand);

    // Update distance window using latest GPS sample
    // actual_run = current run counter
    // Returns best speed (mm/s) for this window
    double Update_distance(int actual_run);

    // -------------------------------------------------------------------------
    // Live calculation state
    // -------------------------------------------------------------------------
    double m_speed        = 0;   // Average speed over full distance window (mm/s)
    double m_speed_alfa   = 0;   // Shortened distance avg (used by Alpha)
    double m_max_speed    = 0;   // Best speed seen in current run

    // -------------------------------------------------------------------------
    // Top-10 storage (sorted descending)
    // -------------------------------------------------------------------------
    double  avg_speed[10]     = {};  // Persistent top speeds
    double  display_speed[10] = {};  // Working copy for live display sorting

    int     m_Distance[10] = {};     // Distance accumulated for each entry (mm)
    uint8_t time_hour[10]  = {};
    uint8_t time_min[10]   = {};
    uint8_t time_sec[10]   = {};

    int this_run[10]   = {};   // Run index associated with each entry
    int nr_samples[10]= {};   // Samples used for each avg
    int message_nr[10]= {};   // UBX message index (trace / SBP)

    // -------------------------------------------------------------------------
    // Sliding window internals
    // -------------------------------------------------------------------------
    int m_index         = 0;   // Start index of window in GPS ring buffer
    int m_distance      = 0;   // Accumulated distance (mm)
    int m_distance_alfa = 0;   // Distance before overshoot (for Alpha)
    int m_set_distance  = 0;   // Configured distance window (meters)
    int m_Set_Distance  = 0;   // Internal target (mm * sample_rate)
    int m_sample        = 0;   // Number of samples in current window

private:
    int old_run = -1;          // Previous run counter (detect run boundary)
};

// -----------------------------------------------------------------------------
// Global distance windows (unchanged API)
// -----------------------------------------------------------------------------
extern GPS_speed M100;    // 100 m
extern GPS_speed M250;    // 250 m
extern GPS_speed M500;    // 500 m
extern GPS_speed M1852;   // 1 nautical mile
