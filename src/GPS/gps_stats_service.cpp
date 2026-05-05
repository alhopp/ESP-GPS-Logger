#include "GPS/gps_stats_service.h"

#include "Core/Globals.h"
#include "GPS/gps_alpha_speed.h"
#include "GPS/gps_run_detector.h"
#include "GPS/gps_distance_speed.h"
#include "GPS/gps_time_speed.h"

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

  gps_run_update(
    fix.headingDeg,
    fix.speedKnots
  );

  run_count = gps_run_current();

  if (gps_run_started()) {
    // reset per-run stats here
  }

  if (run_count != old_run_count) {
    Ublox.run_distance = 0;
  }

  old_run_count = run_count;

  M500.Update_distance(run_count);
  M1852.Update_distance(run_count);

  A500.Update_Alfa(M500);

  S2.Update_speed(run_count);
  S10.Update_speed(run_count);
  S3600.Update_speed(run_count);
}
