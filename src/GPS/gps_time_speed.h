#pragma once
#include <stdint.h>

// ============================================================================
// GPS_time
// Time-window based speed statistics (2s, 10s, 1h)
//
// UNITS
// - All speeds are KNOTS
//
// RESPONSIBILITIES
// - Rolling window averages (SBP-aligned)
// - Session-level best per window
// - Per-run bests (display support only)
// - Ranked top-10 results
// - Avg of best 5 × 10s
//
// ALSO EXPORTS
// - Geometry window indices for GeoJSON generation
// ============================================================================

class GPS_time {
public:
  explicit GPS_time(int tijdvenster);

  // ---------------- per-run (10s only) ----------------
  float best_10s_per_run[32];   // best 10s per completed run
  int   run_count;              // highest run index seen

  // Update statistics for the current run
  // Returns session-level best for this window (knots)
  float Update_speed(int actual_run);

  // Reset all rolling / ranked state (new session)
  void Reset_stats();

  // ---------------- session-level ----------------
  double s_max_speed;        // best speed of entire session (knots)
  double avg_5runs;          // avg of best 5 × 10s (knots, 10s only)

  // ---------------- ranked results ----------------
  double avg_speed[10];      // top-10 session speeds (knots)
  double display_speed[10];  // sorted copy for UI

  // ---------------- metadata ----------------------
  uint8_t time_hour[10];
  uint8_t time_min [10];
  uint8_t time_sec [10];
  int     this_run [10];

  // ---------------- configuration -----------------
  int time_window;           // window length (seconds)

  // ---------------- display helpers ---------------
  float display_max_speed;   // cached max for UI
  float display_last_run;    // optional (set on run end)

private:
  int old_run;
};

// ---------------------------------------------------------------------------
// Global instances (API preserved)
// ---------------------------------------------------------------------------
extern GPS_time S2;     // 2-second window (session best)
extern GPS_time S10;    // 10-second window
extern GPS_time S1800;  // 30-minute window (if used)
extern GPS_time S3600;  // 1-hour window

// ============================================================================
// Geometry window exports (USED BY STORAGE / GEOJSON)
// All indices are GPS sample indices unless stated otherwise.
// A value of -1 means "not valid / not available".
// ============================================================================

// ---- 2s window (GPS index domain) ----
extern int win_2s_start;
extern int win_2s_end;

// ---- 10s window (GPS index domain) ---
extern int win_10s_start;
extern int win_10s_end;

// ---- 1h window (SECOND index domain) --
extern int win_1h_start_sec;
extern int win_1h_end_sec;

// ---- Second → GPS index mapping ------
extern int sec_to_gps_index[];


extern int win_10s_top5_start[5];
extern int win_10s_top5_end[5];
extern int win_10s_top5_count;