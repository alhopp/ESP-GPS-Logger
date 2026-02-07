#pragma once
#include <stdint.h>

class GPS_speed;

// ============================================================================
// Alfa_speed
//
// Calculates “ALFA speed” using a circular geometry constraint.
//
// Definition:
// - Average speed over a fixed distance window (e.g. 250 m / 500 m)
// - Straight-line distance between entry and exit must stay within a
//   circular radius (typically 50 m)
//
// How it works:
// - Uses an existing GPS_speed instance for distance-based speed
// - Computes straight-line distance between current point and window start
// - If distance² < alfa_radius² → valid ALFA
//
// Features:
// - Tracks max ALFA speed per run
// - Stores top-10 ALFA results (sorted)
// - Captures timestamp, run index, UBX message index
// - Auto-resets on run change (external run detection)
//
// Notes:
// - Update_Alfa() must be called every GPS sample
// - Uses global GPS buffers (_lat, _long, index_GPS)
// - Uses squared distances to avoid sqrt()
// ============================================================================





class Alfa_speed {
public:
  // Constructor: alfa radius in meters (e.g. 50 m)
  Alfa_speed(int alfa_radius);

  // Update ALFA calculation using distance-based speed window
  float Update_Alfa(const GPS_speed& M);

  // Reset all stored ALFA statistics
  void Reset_stats(void);

  void  Finalise_Run(void);

  
  // -------------------------------------------------------------------------
  // Live state
  // -------------------------------------------------------------------------
  double straight_dist_square;   // Straight-line distance² (m²)
  double alfa_speed;             // Current ALFA speed (knots)
  double alfa_speed_max;         // Max ALFA speed in current run (knots)
  float  display_max_speed;      // Live display value

  // -------------------------------------------------------------------------
  // Configuration
  // -------------------------------------------------------------------------
  double alfa_circle_square;     // ALFA radius² (m²)

  // -------------------------------------------------------------------------
  // Stored results (top-10)
  // -------------------------------------------------------------------------
  double  avg_speed[10];         // Sorted ALFA speeds (knots)
  int     real_distance[10];     // Straight-line distance (m)

  uint8_t time_hour[10];
  uint8_t time_min[10];
  uint8_t time_sec[10];

  int this_run[10];              // alfa_counter per entry
  int message_nr[10];            // UBX NAV-PVT index
  int alfa_distance[10];         // Path distance inside window (m)

private:
  int old_run_count = -1;        // Detects run transitions
};


// ---------------------------------------------------------------------------
// Global ALFA windows (unchanged API)
// ---------------------------------------------------------------------------
extern Alfa_speed A250;
extern Alfa_speed A500;
extern Alfa_speed a500;
extern float alfa_exit;



// -----------------------------------------------------------------------------
// Alpha indicator helper
//
// Computes the perpendicular distance of the current position relative to
// the alpha reference line defined by two GPS_speed windows (typically 250 m
// and 100 m before the jibe).
//
// Used to determine whether the current position still lies within the
// allowed alpha corridor (e.g. ≤ 50 m).
//
// Parameters:
// - M250 : GPS_speed instance for 250 m window
// - M100 : GPS_speed instance for 100 m window
// - actual_heading : current course heading (degrees)
//
// Returns:
// - Signed perpendicular distance in meters
// -----------------------------------------------------------------------------
float Alfa_indicator(GPS_speed M250,
                     GPS_speed M100,
                     float actual_heading);


// ---------------------------------------------------------------------------
// Geometry window export (Alpha 500)
// ---------------------------------------------------------------------------
extern int alpha_start;
extern int alpha_end;


