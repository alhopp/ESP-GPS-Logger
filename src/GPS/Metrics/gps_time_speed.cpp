#include "GPS/Metrics/gps_time_speed.h"
#include "GPS/Data/gps_data.h"
#include "GPS/Data/gps_runtime_instances.h"
#include "GPS/Data/gps_satellite_quality.h"
#include "GPS/Metrics/gps_result_sort.h"

#include "GPS/Ublox/ublox_driver.h"
#include "Core/Globals.h"
#include "Core/system_info.h"
#include "GPS/gps_runtime_state.h"

#include <time.h>

// ============================================================================
// GPS_time_speed
//
// Time-window speed engine for GPS data.
//
// - Computes RP6/Speedreader-aligned 2s, 10s, and 1h speed metrics
// - Tracks session-best and per-run results (Speedreader compatible)
// - Exports ring-indexed geometry windows for plotting/logging (no rendering)
//
// CONTRACT
// - Inputs: raw GPS speed buffers (_gSpeed[], _secSpeed[])
// - Outputs: mm/s; display/export code converts to knots
// - Outputs: best speeds + window start/end indices
// - index_GPS is a monotonic GPS sample counter
// - Exported windows are absolute GPS sample ranges
//
// ROLE
// - Numbers are computed here
// - Geometry *locations* are identified here
// - Drawing / logging is handled elsewhere
// ============================================================================



// -----------------------------------------------------------------------------
// Geometry window exports (SESSION BEST — kept for backward compatibility)
// -----------------------------------------------------------------------------
int win_2s_start  = -1;

int win_1h_start_sec = -1;
int win_1h_end_sec   = -1;

// -----------------------------------------------------------------------------
// Top-5 10s geometry exports (Speedreader style)
// -----------------------------------------------------------------------------
int win_10s_top5_start[5] = { -1, -1, -1, -1, -1 };
int win_10s_top5_count    = 0;
float win_10s_top5_speed[5] = {0, 0, 0, 0, 0};

// -----------------------------------------------------------------------------
// Per-run 10s storage
// -----------------------------------------------------------------------------
float best_10s_per_run[32];
int   win_10s_start_run[32];

namespace {
void sortTop10sWindows()
{
  for (int i = 0; i < win_10s_top5_count - 1; i++) {
    for (int j = i + 1; j < win_10s_top5_count; j++) {
      if (win_10s_top5_speed[i] < win_10s_top5_speed[j]) {
        const float s = win_10s_top5_speed[i];
        win_10s_top5_speed[i] = win_10s_top5_speed[j];
        win_10s_top5_speed[j] = s;

        const int start = win_10s_top5_start[i];
        win_10s_top5_start[i] = win_10s_top5_start[j];
        win_10s_top5_start[j] = start;
      }
    }
  }
}

bool windowsOverlap(int aStart, int bStart, int samples)
{
  return aStart < bStart + samples && bStart < aStart + samples;
}

void updateTop10sWindows(int start, float speed, int samples)
{
  if (start < 0 || speed <= 0.0f) return;

  for (int i = 0; i < win_10s_top5_count; i++) {
    if (windowsOverlap(start, win_10s_top5_start[i], samples)) {
      if (speed > win_10s_top5_speed[i]) {
        win_10s_top5_speed[i] = speed;
        win_10s_top5_start[i] = start;
        sortTop10sWindows();
      }
      return;
    }
  }

  if (win_10s_top5_count < 5) {
    const int idx = win_10s_top5_count++;
    win_10s_top5_speed[idx] = speed;
    win_10s_top5_start[idx] = start;
    sortTop10sWindows();
    return;
  }

  if (speed <= win_10s_top5_speed[4]) return;

  win_10s_top5_speed[4] = speed;
  win_10s_top5_start[4] = start;
  sortTop10sWindows();
}
}

// -----------------------------------------------------------------------------
GPS_time_speed::GPS_time_speed(int tijdvenster) : time_window(tijdvenster){ Reset_stats(); }

// -----------------------------------------------------------------------------
void GPS_time_speed::Reset_stats()
{
  for(int i=0;i<10;i++){
    avg_speed[i]=0;
    display_speed[i]=0;
    time_hour[i]=0;
    time_min[i]=0;
    time_sec[i]=0;
    Mean_cno[i]=0;
    Max_cno[i]=0;
    Min_cno[i]=0;
    Mean_numSat[i]=0;
    this_run[i]=0;
  }
  avg_5runs=0;
  avg_s=0;
  avg_s_sum=0;
  s_max_speed=0;
  run_count=0;
  old_run=0;
  reset_display_last_run=0;
  display_max_speed=0;
  display_last_run=0;

  for(int i=0;i<32;i++){
    best_10s_per_run[i]=0.0f;
    win_10s_start_run[i]=-1;
  }

  for(int i=0;i<5;i++){
    win_10s_top5_start[i]=-1;
    win_10s_top5_speed[i]=0.0f;
  }
  win_10s_top5_count=0;
}

// -----------------------------------------------------------------------------
float GPS_time_speed::Update_speed(int actual_run)
{
  if(actual_run > run_count && actual_run < 32) run_count = actual_run;

  if(time_window * systemInfo.sample_rate < BUFFER_SIZE){
    const int samples = time_window * systemInfo.sample_rate;
    avg_s_sum += _gSpeed[index_GPS % BUFFER_SIZE];

    if(index_GPS >= samples){
      avg_s_sum -= _gSpeed[(index_GPS - samples) % BUFFER_SIZE];
    }

    if(time_window == 10){
      const int start = index_GPS - samples + 1;
      updateTop10sWindows(start, avg_s, samples);
    }

    avg_s = avg_s_sum / time_window / systemInfo.sample_rate;

    if(s_max_speed < avg_s){
      s_max_speed = avg_s;

      const int start = index_GPS - samples + 1;
      if(time_window == 2){ win_2s_start = start; }

      getLocalTime(&tmstruct,0);
      time_hour[0]=tmstruct.tm_hour;
      time_min[0]=tmstruct.tm_min;
      time_sec[0]=tmstruct.tm_sec;
      this_run[0]=actual_run;
      avg_speed[0]=s_max_speed;
      Mean_cno[0]=Ublox_Sat.sat_info.Mean_mean_cno;
      Max_cno[0]=Ublox_Sat.sat_info.Mean_max_cno;
      Min_cno[0]=Ublox_Sat.sat_info.Mean_min_cno;
      Mean_numSat[0]=Ublox_Sat.sat_info.Mean_numSV;

      if(time_window == 10 && actual_run > 0 && actual_run < 32){
        best_10s_per_run[actual_run] = s_max_speed;
        win_10s_start_run[actual_run] = start;
      }

      for(int i=0;i<10;i++) display_speed[i]=avg_speed[i];
      sort_display(display_speed,10);
      display_max_speed = display_speed[9];
      avg_5runs=0;
      for(int i=5;i<10;i++) avg_5runs += display_speed[i];
      avg_5runs /= 5;
    }

    if((actual_run != old_run) && (this_run[0] == old_run)){
      sort_run(
        avg_speed,
        time_hour,
        time_min,
        time_sec,
        Mean_cno,
        Max_cno,
        Min_cno,
        Mean_numSat,
        this_run,
        10
      );

      avg_speed[0]=0;
      s_max_speed=0;
      avg_5runs=0;
      for(int i=5;i<10;i++) avg_5runs += avg_speed[i];
      avg_5runs /= 5;

      for(int i=0;i<10;i++) display_speed[i]=avg_speed[i];
      sort_display(display_speed,10);
      display_max_speed = display_speed[9];
    }

    if((actual_run != reset_display_last_run) && (avg_s > 3000)){
      reset_display_last_run = actual_run;
      display_last_run = 0;
    }else if(display_last_run < s_max_speed){
      display_last_run = s_max_speed;
    }

    old_run = actual_run;
    return s_max_speed;
  }

  if(index_GPS % systemInfo.sample_rate == 0){
    avg_s_sum += (int)_secSpeed[index_sec % BUFFER_SIZE];
    if(index_sec >= time_window){
      avg_s_sum -= (int)_secSpeed[(index_sec - time_window) % BUFFER_SIZE];
    }
    avg_s = avg_s_sum / time_window;

    if(s_max_speed < avg_s){
      s_max_speed = avg_s;
      getLocalTime(&tmstruct,0);
      time_hour[0]=tmstruct.tm_hour;
      time_min[0]=tmstruct.tm_min;
      time_sec[0]=tmstruct.tm_sec;
      this_run[0]=actual_run;
      avg_speed[0]=s_max_speed;

      if(time_window == 3600){
        win_1h_end_sec = index_sec;
        win_1h_start_sec = index_sec - time_window + 1;
        if(win_1h_start_sec < 0) win_1h_start_sec = 0;
      }
    }

    if(s_max_speed > avg_speed[9]) display_max_speed = s_max_speed;
    else display_max_speed = avg_speed[9];

    if((actual_run != old_run) && (this_run[0] == old_run)){
      sort_run(
        avg_speed,
        time_hour,
        time_min,
        time_sec,
        Mean_cno,
        Max_cno,
        Min_cno,
        Mean_numSat,
        this_run,
        10
      );
      avg_speed[0]=0;
      s_max_speed=0;
      avg_5runs=0;
      for(int i=5;i<10;i++) avg_5runs += avg_speed[i];
      avg_5runs /= 5;
    }

    old_run = actual_run;
    return s_max_speed;
  }

  return s_max_speed;
}
