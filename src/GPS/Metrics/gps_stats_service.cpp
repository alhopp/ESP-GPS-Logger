#include "GPS/Metrics/gps_stats_service.h"

#include "Core/Globals.h"
#include "GPS/Data/gps_data.h"
#include "GPS/Metrics/gps_alpha_speed.h"
#include "GPS/Metrics/gps_run_detector.h"
#include "GPS/Metrics/gps_distance_speed.h"
#include "GPS/Metrics/gps_time_speed.h"

// Coordinates one NAV-PVT sample through the GPS statistics pipeline.
// Keep ordering explicit here: sample ingestion/filtering feeds the legacy RP6
// distance, time, run, and alpha calculators without each module knowing about
// the others.

namespace {
bool have_last_good_heading = false;
float last_good_heading = 0.0f;

bool isDuplicateNavPvt()
{
  if (nav_pvt_message == old_message) return true;

  old_message = nav_pvt_message;
  return false;
}

void ingestSample(const GpsFix& fix)
{
  gps_speed_value = ubxMessage.navPvt.gSpeed; // mm/s
  Ublox.push_data(fix.lat, fix.lon, gps_speed_value);
}

bool currentSampleGood()
{
  return _sampleGood[index_GPS % BUFFER_SIZE];
}

void updateHeadingForRunDetection(const GpsFix& fix, bool sample_good)
{
  if (sample_good) {
    last_good_heading = fix.headingDeg;
    have_last_good_heading = true;
  }
}

void updateRunDetection(bool sample_good)
{
  if (sample_good || have_last_good_heading) {
    gps_run_update(last_good_heading, S2.avg_s);
  }

  run_count = gps_run_current();

  if (run_count != old_run_count) {
    Ublox.run_distance = 0;
  }

  old_run_count = run_count;
}

void updateDistanceWindows()
{
  M100.Update_distance(run_count);
  M250.Update_distance(run_count);
  M500.Update_distance(run_count);
  M1852.Update_distance(run_count);
}

void updateTimeWindows()
{
  S2.Update_speed(run_count);
  s2.Update_speed(run_count);
  S10.Update_speed(run_count);
  s10.Update_speed(run_count);
  S1800.Update_speed(run_count);
  S3600.Update_speed(run_count);
}

void updateAlphaWindows()
{
  A250.Update_Alfa(M250);
  A500.Update_Alfa(M500);
  a500.Update_Alfa(M500);
}
}

void gps_stats_service_reset()
{
  have_last_good_heading = false;
  last_good_heading = 0.0f;
}

void gps_stats_update(const GpsFix& fix)
{
  if (isDuplicateNavPvt()) return;

  // RP6 update order:
  // ingest/filter sample -> run detection using previous S2.avg_s
  // -> distance windows -> time windows -> alpha windows.
  ingestSample(fix);

  const bool sample_good = currentSampleGood();
  updateHeadingForRunDetection(fix, sample_good);
  updateRunDetection(sample_good);

  updateDistanceWindows();
  updateTimeWindows();
  updateAlphaWindows();
}
