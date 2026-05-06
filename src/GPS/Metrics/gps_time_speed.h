#pragma once
#include <stdint.h>

// ============================================================================
// GPS_time_speed
// Time-window based speed statistics (2s, 10s, 1h)
//
// UNITS
// - All speeds are mm/s. Display/export code converts to knots.
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

class GPS_time_speed {
public:
  explicit GPS_time_speed(int tijdvenster);

  // ---------------- per-run / rolling state ----------------
  float best_10s_per_run[32];   // compatibility export for RTC/GeoJSON
  int   run_count;              // highest run index seen

  // Update statistics for the current run
  // Returns session-level best for this window (mm/s)
  float Update_speed(int actual_run);

  // Reset all rolling / ranked state (new session)
  void Reset_stats();

  // ---------------- session-level ----------------
  double s_max_speed;        // best speed of entire session (mm/s)
  double avg_5runs;          // avg of best 5 x 10s (mm/s, 10s only)
  double avg_s;              // current rolling average (mm/s)
  double avg_s_sum;          // rolling sum backing avg_s

  // ---------------- ranked results ----------------
  double avg_speed[10];      // top-10 session speeds (mm/s)
  double display_speed[10];  // sorted copy for UI

  // ---------------- metadata ----------------------
  uint8_t time_hour[10];
  uint8_t time_min [10];
  uint8_t time_sec [10];
  uint8_t Mean_cno[10];
  uint8_t Max_cno[10];
  uint8_t Min_cno[10];
  uint8_t Mean_numSat[10];
  int     this_run [10];

  // ---------------- configuration -----------------
  int time_window;           // window length (seconds)

  // ---------------- display helpers ---------------
  float display_max_speed;   // cached max for UI
  float display_last_run;    // optional (set on run end)

private:
  int old_run;
  int reset_display_last_run;
};

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
