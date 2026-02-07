#pragma once
#include <stdint.h>

// ============================================================================
// GPS_speed
// Distance-based average speed calculator (Speedreader-aligned)
//
// USED FOR:
//   - 100 m, 250 m, 500 m, 1852 m (1 NM)
//   - Best speed detection per window
//   - Alpha speed support (via m_speed_alfa)
//
// UNIT MODEL:
//   - Distance integration : mm per sample (_gSpeed / sample_rate)
//   - Speed samples        : cm/s → knots BEFORE averaging
//   - Sample 0             : state init ONLY (no distance contribution)
//   - Padding allowed for incomplete windows
//
// DATA SOURCE:
//   - Global GPS ring buffers (_gSpeed, _sogCms, index_GPS)
//
// OWNERSHIP:
//   - Each GPS_speed instance owns *one* distance window
//   - Geometry export is owned by the instance that wins
// ============================================================================

class GPS_speed {
public:
  // afstand = distance window in meters (e.g. 100 / 250 / 500 / 1852)
  explicit GPS_speed(int afstand);

  // -------------------------------------------------------------------------
  // Update distance window using latest GPS sample
  //
  // actual_run = current run counter
  // Returns    = session-level best speed for this window (KNOTS)
  // -------------------------------------------------------------------------
  double Update_distance(int actual_run);

  // -------------------------------------------------------------------------
  // Live calculation state (KNOTS)
  // -------------------------------------------------------------------------
  double m_speed       = 0.0;   // Avg speed over full distance window
  double m_speed_alfa  = 0.0;   // Shortened distance avg (used by Alpha)
  double m_max_speed   = 0.0;   // Best speed seen (session-level)

  // -------------------------------------------------------------------------
  // Top-10 storage (session-level, sorted descending)
  // -------------------------------------------------------------------------
  double  avg_speed[10]     = {};   // Persistent top speeds
  double  display_speed[10] = {};   // Working copy for UI sorting

  int     m_Distance[10] = {};      // Distance accumulated for each entry (mm)
  uint8_t time_hour[10]  = {};
  uint8_t time_min[10]   = {};
  uint8_t time_sec[10]   = {};

  int this_run[10]    = {};         // Run index associated with each entry
  int nr_samples[10]  = {};         // Samples used for each avg
  int message_nr[10]  = {};         // UBX message index (trace / SBP)

  // -------------------------------------------------------------------------
  // Sliding window internals
  // -------------------------------------------------------------------------
  int m_index         = 0;   // Start index of window in GPS ring buffer
  int m_distance      = 0;   // Accumulated distance (mm)
  int m_distance_alfa = 0;   // Distance before overshoot (for Alpha)
  int m_set_distance  = 0;   // Configured distance window (meters)
  int m_Set_Distance  = 0;   // Internal target distance (mm)
  int m_sample        = 0;   // Number of distance-bearing samples

private:
  int old_run = -1;          // Previous run counter (detect run boundary)
};

// ============================================================================
// Geometry window exports (USED BY STORAGE / GEOJSON)
//
// Indices are GPS sample indices into _lat[] / _long[] buffers.
// A value of -1 means "not valid / not available".
// ============================================================================

// ---- Nautical Mile (1852 m) ----
extern int win_nm_start;
extern int win_nm_end;

// ============================================================================
// Global distance window instances (API preserved)
// ============================================================================
extern GPS_speed M100;     // 100 m
extern GPS_speed M250;     // 250 m
extern GPS_speed M500;     // 500 m
extern GPS_speed M1852;    // 1 nautical mile
