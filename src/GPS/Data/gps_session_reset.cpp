#include "GPS/Data/gps_session_reset.h"

#include "Core/Globals.h"
#include "GPS/Data/gps_data.h"
#include "GPS/Metrics/gps_alpha_speed.h"
#include "GPS/Metrics/gps_distance_speed.h"
#include "GPS/Metrics/gps_run_detector.h"
#include "GPS/Metrics/gps_stats_service.h"
#include "GPS/Metrics/gps_time_speed.h"

void reset_session_stats()
{
  total_distance = 0;
  Ublox.run_distance = 0;
  Ublox.alfa_distance = 0;
  run_count = 0;
  old_run_count = 0;
  alfa_counter = 0;
  gps_run_reset();
  gps_stats_service_reset();
  gps_data_reset_quality_state();

  S2.Reset_stats();
  s2.Reset_stats();
  S10.Reset_stats();
  s10.Reset_stats();
  S1800.Reset_stats();
  S3600.Reset_stats();
  A250.Reset_stats();
  A500.Reset_stats();
  a500.Reset_stats();

  GPS_speed* distance_windows[] = { &M100, &M250, &M500, &M1852 };
  for (GPS_speed* w : distance_windows) {
    w->m_speed = 0.0;
    w->m_speed_alfa = 0.0;
    w->m_max_speed = 0.0;
    w->m_distance = 0;
    w->m_distance_alfa = 0;
    w->m_index = 0;
    w->m_sample = 0;
    for (int i = 0; i < 10; i++) {
      w->avg_speed[i] = 0.0;
      w->display_speed[i] = 0.0;
      w->m_Distance[i] = 0;
      w->time_hour[i] = 0;
      w->time_min[i] = 0;
      w->time_sec[i] = 0;
      w->this_run[i] = 0;
      w->nr_samples[i] = 0;
      w->message_nr[i] = 0;
    }
  }

  nav_pvt_message = 0;
  old_message = -1;
}
