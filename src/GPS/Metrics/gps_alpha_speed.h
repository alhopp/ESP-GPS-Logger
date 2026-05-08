#pragma once

#include <stdint.h>

class GPS_distance_speed;

// ============================================================================
// Alfa_speed
//
// Calculates alpha speed using the RP6/Speedreader model:
// - average speed comes from a distance-window calculator, normally 500 m
// - the entry and exit points must close within the configured radius, normally
//   50 m for Alpha 500
// - results are sorted per run and exported for RTC/display/GeoJSON
// ============================================================================

class Alfa_speed {
public:
  explicit Alfa_speed(int alfa_radius);

  float Update_Alfa(const GPS_distance_speed& M);
  void Reset_stats();
  void Finalise_Run();

  double straight_dist_square;   // Straight-line distance squared, m^2.
  double alfa_speed;             // Current alpha speed, mm/s.
  double alfa_speed_max;         // Best alpha speed in current run, mm/s.
  float display_max_speed;       // Live display value, mm/s.

  double alfa_circle_square;     // Allowed closure radius squared, m^2.

  double avg_speed[10];          // Sorted alpha speeds, mm/s.
  int real_distance[10];         // Squared straight-line distance, m^2.

  uint8_t time_hour[10];
  uint8_t time_min[10];
  uint8_t time_sec[10];

  int this_run[10];              // Run counter per entry.
  int message_nr[10];            // UBX NAV-PVT message index.
  int alfa_distance[10];         // Path distance inside window, mm.

private:
  int old_run_count = -1;
};

// Geometry/stat export for the session-best Alpha 500.
extern int alpha_start;
extern int alpha_end;
extern int alpha_sbp_start;
extern int alpha_sbp_end;
extern float alpha_best_speed_mmps;
extern float alpha_best_closure_m;
extern int alpha_best_distance_m;
