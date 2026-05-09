#include "GPS/Data/gps_session_reset.h"

// ============================================================================
// gps_session_reset.cpp
//
// Central reset point for the GPS statistics graph.
//
// Keeping this together prevents old run/alpha/window state leaking into a new
// session after sleep, config changes, or manual restart.
// ============================================================================

#include "Core/Globals.h"
#include "GPS/Data/gps_data.h"
#include "GPS/Data/gps_runtime_instances.h"
#include "GPS/gps_runtime_state.h"
#include "GPS/Metrics/gps_alpha_guidance.h"
#include "GPS/Metrics/gps_run_detector.h"
#include "GPS/Metrics/gps_stats_service.h"

namespace {
void resetDistanceWindow(GPS_distance_speed& window)
{
  window.m_speed = 0.0;
  window.m_speed_alfa = 0.0;
  window.m_max_speed = 0.0;
  window.m_distance = 0;
  window.m_distance_alfa = 0;
  window.m_index = 0;
  window.m_sample = 0;

  for (int i = 0; i < 10; i++) {
    window.avg_speed[i] = 0.0;
    window.display_speed[i] = 0.0;
    window.m_Distance[i] = 0;
    window.time_hour[i] = 0;
    window.time_min[i] = 0;
    window.time_sec[i] = 0;
    window.this_run[i] = 0;
    window.nr_samples[i] = 0;
    window.message_nr[i] = 0;
  }
}
} // namespace

void reset_session_stats()
{
  // Raw distance/run counters that are shared across metric modules.
  total_distance = 0;
  Ublox.run_distance = 0;
  Ublox.alfa_distance = 0;
  run_count = 0;
  old_run_count = 0;
  alfa_counter = 0;

  // Clear stateful services that remember previous samples or runs.
  gps_run_reset();
  gps_stats_service_reset();
  gps_alpha_guidance_reset();
  gps_data_reset_quality_state();

  // Time and alpha classes own their own reset routines.
  speed_2s.Reset_stats();
  speed_10s.Reset_stats();
  speed_1h.Reset_stats();
  alpha_500m.Reset_stats();
#if STATS_ONLY_SERIAL
  alpha_500m_60.Reset_stats();
  alpha_500m_70.Reset_stats();
#endif

  // Distance windows still expose their live arrays directly for legacy RP6
  // compatibility, so reset them here until the class owns a Reset_stats().
  GPS_distance_speed* distance_windows[] = { &speed_100m, &speed_250m, &speed_500m, &speed_nm };
  for (GPS_distance_speed* w : distance_windows) {
    resetDistanceWindow(*w);
  }
  gps_distance_speed_reset_session_windows();

  // Duplicate NAV-PVT suppression should restart cleanly with the session.
  nav_pvt_message = 0;
}
