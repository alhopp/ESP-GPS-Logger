#include "GPS/Metrics/gps_run_detector.h"

#include <Arduino.h>
#include <math.h>

#include "Core/Globals.h"
#include "Core/build_config.h"
#include "Core/system_info.h"
#include "GPS/Data/gps_data.h"
#include "GPS/gps_config.h"

// Detects Speedreader-style runs and jibes from heading stability plus 2s speed.
// The rest of the metric code consumes the resulting run number and jibe index;
// this module owns only that detector state.

namespace {
constexpr int SPEED_DETECTION_MIN = 4000;       // mm/s
constexpr int STANDSTILL_DETECTION_MAX = 1000;  // mm/s
constexpr int MEAN_HEADING_TIME = 15;           // seconds
constexpr int STRAIGHT_COURSE_MAX_DEV = 10;     // degrees
constexpr int JIBE_COURSE_DEVIATION_MIN = 50;   // degrees

float old_heading = 0.0f;
float delta_heading = 0.0f;
float heading = 0.0f;
float mean_heading = 0.0f;
uint32_t delay_counter = 0;
int run_counter = 0;
bool velocity_0 = false;
bool velocity_5 = false;
bool straight_course = false;

bool run_started_flag = false;
bool run_ended_flag = false;
int last_jibe_idx = -1;
int armed_count = 0;
int jibe_count = 0;
int standstill_restart_count = 0;
int last_armed_idx = -1;

float unwrapHeading(float actual_heading)
{
  if ((actual_heading - old_heading) > 300.0f) delta_heading -= 360.0f;
  if ((actual_heading - old_heading) < -300.0f) delta_heading += 360.0f;

  old_heading = actual_heading;
  heading = actual_heading + delta_heading;
  return heading;
}

void printRunDebug(const char* event,
                   float actual_heading,
                   float unwrapped_heading,
                   float heading_deviation,
                   float s2_speed_mmps)
{
#if RUN_DETECTOR_DEBUG
  Serial.printf(
    "[RUNDBG] %-10s idx=%d run=%d alfa=%d raw=%.1f head=%.1f mean=%.1f dev=%.1f s2=%.0f delay=%lu straight=%d v5=%d v0=%d\n",
    event,
    index_GPS,
    run_counter,
    alfa_counter,
    actual_heading,
    unwrapped_heading,
    mean_heading,
    heading_deviation,
    s2_speed_mmps,
    static_cast<unsigned long>(delay_counter),
    straight_course ? 1 : 0,
    velocity_5 ? 1 : 0,
    velocity_0 ? 1 : 0
  );
#else
  (void)event;
  (void)actual_heading;
  (void)unwrapped_heading;
  (void)heading_deviation;
  (void)s2_speed_mmps;
#endif
}
}

void gps_run_reset()
{
  delay_counter = 0;
  run_counter = 0;
  velocity_0 = false;
  velocity_5 = false;
  straight_course = false;
  run_started_flag = false;
  run_ended_flag = false;
  last_jibe_idx = -1;
  armed_count = 0;
  jibe_count = 0;
  standstill_restart_count = 0;
  last_armed_idx = -1;
  old_heading = 0.0f;
  delta_heading = 0.0f;
  heading = 0.0f;
  mean_heading = 0.0f;
}

void gps_run_update(float actual_heading, float s2_speed_mmps)
{
  run_started_flag = false;
  run_ended_flag = false;

  const int sample_rate = systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 5;
  const int speed_detection_min = SPEED_DETECTION_MIN;
  const int standstill_detection_max = STANDSTILL_DETECTION_MAX;
  const int mean_heading_time = MEAN_HEADING_TIME;
  const int straight_course_max = STRAIGHT_COURSE_MAX_DEV;
  const int course_deviation_min = JIBE_COURSE_DEVIATION_MIN;
  const int time_delay_new_run = TIME_DELAY_NEW_RUN;
  const int mean_samples = mean_heading_time * sample_rate;

  const float unwrapped_heading = unwrapHeading(actual_heading);
  mean_heading =
    mean_heading * (mean_samples - 1) / mean_samples +
    unwrapped_heading / mean_samples;
  const float heading_deviation = fabsf(mean_heading - unwrapped_heading);

  if (s2_speed_mmps > speed_detection_min) velocity_5 = true;
  if ((s2_speed_mmps < standstill_detection_max) && velocity_5) velocity_0 = true;

  if (velocity_0 && (s2_speed_mmps > speed_detection_min)) {
    velocity_5 = false;
    velocity_0 = false;
    delay_counter = (time_delay_new_run - 1) * sample_rate;
    standstill_restart_count++;
    printRunDebug("standstill", actual_heading, unwrapped_heading, heading_deviation, s2_speed_mmps);
  }

  if ((heading_deviation < straight_course_max) &&
      (s2_speed_mmps > speed_detection_min)) {
    if (!straight_course) {
      armed_count++;
      last_armed_idx = index_GPS;
      printRunDebug("armed", actual_heading, unwrapped_heading, heading_deviation, s2_speed_mmps);
    }
    straight_course = true;
  }

  if ((heading_deviation > course_deviation_min) && straight_course) {
    printRunDebug("jibe", actual_heading, unwrapped_heading, heading_deviation, s2_speed_mmps);
    straight_course = false;
    delay_counter = 0;
    alfa_counter++;
    jibe_count++;
    run_ended_flag = true;
    last_jibe_idx = index_GPS;
  }

  const uint32_t run_delay_samples = time_delay_new_run * sample_rate;
  delay_counter++;
  if (delay_counter == run_delay_samples) {
    run_counter++;
    run_started_flag = true;
    printRunDebug("run_start", actual_heading, unwrapped_heading, heading_deviation, s2_speed_mmps);
    mean_heading = unwrapped_heading;
    straight_course = s2_speed_mmps > speed_detection_min;
    if (straight_course) {
      armed_count++;
      last_armed_idx = index_GPS;
    }
  }
}

int gps_run_current() { return run_counter; }
bool gps_run_started() { return run_started_flag; }
bool gps_run_ended() { return run_ended_flag; }
int gps_run_last_jibe_index() { return last_jibe_idx; }
int gps_run_armed_count() { return armed_count; }
int gps_run_jibe_count() { return jibe_count; }
int gps_run_standstill_restart_count() { return standstill_restart_count; }
int gps_run_last_armed_index() { return last_armed_idx; }
