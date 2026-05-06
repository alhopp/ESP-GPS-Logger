#include "GPS/Metrics/gps_stats_service.h"

#include "Core/Globals.h"
#include "GPS/Data/gps_data.h"
#include "GPS/Metrics/gps_alpha_speed.h"
#include "GPS/Metrics/gps_run_detector.h"
#include "GPS/Metrics/gps_distance_speed.h"
#include "GPS/Metrics/gps_time_speed.h"

namespace {
bool have_last_good_heading = false;
float last_good_heading = 0.0f;
}

void gps_stats_service_reset()
{
  have_last_good_heading = false;
  last_good_heading = 0.0f;
}

void gps_stats_update(const GpsFix& fix)
{
  if (nav_pvt_message == old_message) return;

  old_message = nav_pvt_message;

  gps_speed_value = ubxMessage.navPvt.gSpeed;   // mm/s

  Ublox.push_data(
    fix.lat,
    fix.lon,
    gps_speed_value
  );

  const bool sample_good = _sampleGood[index_GPS % BUFFER_SIZE];

  if (sample_good) {
    last_good_heading = fix.headingDeg;
    have_last_good_heading = true;
  }

  if (sample_good || have_last_good_heading) {
    gps_run_update(last_good_heading, S2.avg_s);
  }

  run_count = gps_run_current();

  if (gps_run_started()) {
    // reset per-run stats here
  }

  if (run_count != old_run_count) {
    Ublox.run_distance = 0;
  }

  old_run_count = run_count;

  M100.Update_distance(run_count);
  M250.Update_distance(run_count);
  M500.Update_distance(run_count);
  M1852.Update_distance(run_count);

  S2.Update_speed(run_count);
  s2.Update_speed(run_count);
  S10.Update_speed(run_count);
  s10.Update_speed(run_count);
  S1800.Update_speed(run_count);
  S3600.Update_speed(run_count);

  A250.Update_Alfa(M250);
  A500.Update_Alfa(M500);
  a500.Update_Alfa(M500);
}
