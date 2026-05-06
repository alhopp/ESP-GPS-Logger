#include "GPS/Metrics/gps_run_detector.h"

#include <Arduino.h>
#include <math.h>

#include "Core/Globals.h"
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
float delta_heading_local = 0.0f;
float heading = 0.0f;
float mean_heading = 0.0f;
float heading_snapshot = 0.0f;
uint32_t delay_counter = 0;
int run_counter = 0;
bool velocity_0 = false;
bool velocity_5 = false;
bool straight_course = false;

bool run_started_flag = false;
bool run_ended_flag = false;
int last_jibe_idx = -1;
}

void gps_run_reset()
{
  old_heading = 0.0f;
  delta_heading_local = 0.0f;
  heading = 0.0f;
  delay_counter = 0;
  run_counter = 0;
  velocity_0 = false;
  velocity_5 = false;
  straight_course = false;
  run_started_flag = false;
  run_ended_flag = false;
  last_jibe_idx = -1;
  mean_heading = 0.0f;
  heading_snapshot = 0.0f;
}

void gps_run_update(float actual_heading, float s2_speed_mmps)
{
  run_started_flag = false;
  run_ended_flag = false;

  if ((actual_heading - old_heading) > 300.0f) delta_heading_local -= 360.0f;
  if ((actual_heading - old_heading) < -300.0f) delta_heading_local += 360.0f;
  old_heading = actual_heading;
  heading = actual_heading + delta_heading_local;

  const int sample_rate = systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 5;
  const int speed_detection_min = SPEED_DETECTION_MIN;
  const int standstill_detection_max = STANDSTILL_DETECTION_MAX;
  const int mean_heading_time = MEAN_HEADING_TIME;
  const int straight_course_max = STRAIGHT_COURSE_MAX_DEV;
  const int course_deviation_min = JIBE_COURSE_DEVIATION_MIN;
  const int time_delay_new_run = TIME_DELAY_NEW_RUN;

  heading_snapshot = heading;
  mean_heading =
    mean_heading * (mean_heading_time * sample_rate - 1) / (mean_heading_time * sample_rate) +
    heading / (mean_heading_time * sample_rate);

  if (s2_speed_mmps > speed_detection_min) velocity_5 = true;
  if ((s2_speed_mmps < standstill_detection_max) && velocity_5) velocity_0 = true;

  if (velocity_0 && (s2_speed_mmps > speed_detection_min)) {
    velocity_5 = false;
    velocity_0 = false;
    delay_counter = (time_delay_new_run - 1) * sample_rate;
  }

  if ((fabsf(mean_heading - heading) < straight_course_max) &&
      (s2_speed_mmps > speed_detection_min)) {
    straight_course = true;
  }

  if ((fabsf(mean_heading - heading) > course_deviation_min) && straight_course) {
    straight_course = false;
    delay_counter = 0;
    alfa_counter++;
    run_ended_flag = true;
    last_jibe_idx = index_GPS;
  }

  delay_counter++;
  if (delay_counter == (uint32_t)(time_delay_new_run * sample_rate)) {
    run_counter++;
    run_started_flag = true;
  }
}

int gps_run_current() { return run_counter; }
bool gps_run_started() { return run_started_flag; }
bool gps_run_ended() { return run_ended_flag; }
int gps_run_last_jibe_index() { return last_jibe_idx; }
