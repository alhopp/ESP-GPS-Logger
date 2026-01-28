#pragma once
#include <stdint.h>

// ============================================================================
// GPS_time
// Time-window based speed statistics (2s, 10s, 30m, 1h, …)
//
// - Consumes Doppler samples from GPS_data (_gSpeed / _secSpeed)
// - Maintains rolling averages over a fixed time window
// - Tracks session-best, per-run best, and ranked top-10 values
// ============================================================================
class GPS_time {
public:
  // Time window in seconds (e.g. 2, 10, 1800, 3600)
  explicit GPS_time(int tijdvenster);

  // Update statistics for the current run, returns current max (mm/s)
  float Update_speed(int actual_run);

  // Reset all rolling / ranked state (new session)
  void Reset_stats();

  // ---------------- live state ----------------
  double avg_s;            // current averaged speed (mm/s)
  int    avg_s_sum;        // rolling sum accumulator
  double s_max_speed;      // max speed in current run (mm/s)

  float  display_max_speed;// live max shown on display
  float  display_last_run; // max of last completed run

  // ---------------- ranked results ----------------
  double avg_speed[10];    // sorted top-10 speeds (session)
  double display_speed[10];// scratch array for display sorting
  double avg_5runs;        // mean of best 5 speeds

  // ---------------- metadata ----------------
  uint8_t time_hour[10];   // timestamps of ranked results
  uint8_t time_min [10];
  uint8_t time_sec [10];
  int     this_run [10];   // run index per result

  // ---------------- configuration ----------------
  int time_window;         // window length (seconds)

  // ---------------- bar-graph bookkeeping ----------------
  int      speed_run_counter;
  uint16_t speed_run[50];

  // ---------------- GNSS quality at peak ----------------
  uint8_t Mean_cno[10];
  uint8_t Max_cno [10];
  uint8_t Min_cno [10];
  uint8_t Mean_numSat[10];

private:
  int old_run;                 // previous run index
  int reset_display_last_run;  // display reset guard
};

// ---------------------------------------------------------------------------
// Global instances (API preserved)
// ---------------------------------------------------------------------------
extern GPS_time S2;     // 2-second window (session best)
extern GPS_time s2;     // 2-second window (per-run)
extern GPS_time S10;    // 10-second window (session best)
extern GPS_time s10;    // 10-second window (per-run)
extern GPS_time S1800;  // 30-minute window
extern GPS_time S3600;  // 1-hour window
