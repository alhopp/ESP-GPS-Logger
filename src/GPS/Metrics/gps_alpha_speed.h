#pragma once

#include <stdint.h>

class GPS_distance_speed;

// ============================================================================
// Alfa_speed
//
// Calculates alpha speed using the GPS Speedreader-style Alpha 500 model:
// - scan all possible start/end windows inside the history buffer
// - sailed path must be <= the configured distance, normally 500 m
// - finish must close within the configured radius, normally 50 m
// - results are sorted and exported for RTC/display/GeoJSON
// ============================================================================

class Alfa_speed {
public:
  explicit Alfa_speed(int alfa_radius, bool export_best = true);

  float Update_Alfa(const GPS_distance_speed& M);
  void Reset_stats();
  void Finalise_Run();
  int ResultSbpStart(int slot) const;
  int ResultSbpEnd(int slot) const;

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
  void clearResults();
  void sortResults();
  void recordCandidate(double speedMmps,
                       int startGpsIndex,
                       int endGpsIndex,
                       double closureDist2,
                       double distanceScaled,
                       int sampleRate);

  int result_start[10];
  int result_end[10];
  int result_sbp_start[10];
  int result_sbp_end[10];
  int old_run_count = -1;
  bool export_best = true;
};

// Geometry/stat export for the session-best Alpha 500.
extern int alpha_start;
extern int alpha_end;
extern int alpha_sbp_start;
extern int alpha_sbp_end;
extern float alpha_best_speed_mmps;
extern float alpha_best_closure_m;
extern int alpha_best_distance_m;
