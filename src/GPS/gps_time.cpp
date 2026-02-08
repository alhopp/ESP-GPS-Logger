#include "GPS/gps_time.h"
#include "GPS/GPS_data.h"
#include "GPS/gps_utils.h"

#include "Ublox/ublox.h"
#include "Core/Globals.h"
#include "core/system_info.h"

#include <time.h>

// ============================================================================
// GPS_time
//
// SBP-aligned time-window speed statistics
//
// UNIT MODEL
// - Raw input        : cm/s (_sogCms[], _secSpeed[])
// - Per-sample       : converted to knots FIRST
// - Averaging        : float knots (SBP-exact)
// - Storage          : knots
//
// WINDOWS
// - 2s   = 2  * sample_rate (5 Hz)
// - 10s  = 10 * sample_rate (5 Hz)
// - 1h   = 3600 samples (1 Hz, padded)
// ============================================================================


// -----------------------------------------------------------------------------
// Geometry window exports (SESSION BEST — kept for backward compatibility)
// -----------------------------------------------------------------------------
int win_2s_start  = -1;
int win_2s_end    = -1;

int win_10s_start = -1;
int win_10s_end   = -1;

int win_1h_start_sec = -1;
int win_1h_end_sec   = -1;

// -----------------------------------------------------------------------------
// Top-5 10s geometry exports (Speedreader style)
// -----------------------------------------------------------------------------
int win_10s_top5_start[5] = { -1, -1, -1, -1, -1 };
int win_10s_top5_end  [5] = { -1, -1, -1, -1, -1 };
int win_10s_top5_count    = 0;

// -----------------------------------------------------------------------------
// Per-run 10s storage
// -----------------------------------------------------------------------------
float best_10s_per_run[32];
int   win_10s_start_run[32];
int   win_10s_end_run  [32];


// -----------------------------------------------------------------------------
GPS_time::GPS_time(int tijdvenster) : time_window(tijdvenster){ Reset_stats(); }

// -----------------------------------------------------------------------------
void GPS_time::Reset_stats()
{
  for(int i=0;i<10;i++){ avg_speed[i]=0; display_speed[i]=0; }
  avg_5runs=0; s_max_speed=0; run_count=0;

  for(int i=0;i<32;i++){
    best_10s_per_run[i]=0.0f;
    win_10s_start_run[i]=-1;
    win_10s_end_run  [i]=-1;
  }

  for(int i=0;i<5;i++){ win_10s_top5_start[i]=-1; win_10s_top5_end[i]=-1; }
  win_10s_top5_count=0;
}

// -----------------------------------------------------------------------------
float GPS_time::Update_speed(int actual_run)
{
  if(actual_run > run_count && actual_run < 32) run_count = actual_run;

  // ========================================================================
  // 1 HOUR (3600 s) — padded 1 Hz average
  // ========================================================================
  if(time_window == 3600){
    int secs = index_sec;
    if(secs <= 0) return s_max_speed;
    if(secs > 3600) secs = 3600;

    float sum_kn=0.0f;
    for(int i=0;i<secs;i++){
      int idx=(index_sec - i) % BUFFER_SIZE; if(idx<0) idx += BUFFER_SIZE;
      sum_kn += _secSpeed[idx] * CMPS_TO_KNOTS;
    }

    float avg_kn = sum_kn / 3600.0f;

    if(avg_kn > s_max_speed){
      s_max_speed = avg_kn;
      win_1h_end_sec   = index_sec;
      win_1h_start_sec = index_sec - secs + 1;
      if(win_1h_start_sec < 0) win_1h_start_sec = 0;
    }
    return s_max_speed;
  }

  // ========================================================================
  // 2s / 10s — SBP-style rolling window (5 Hz)
  // ========================================================================
  const uint32_t samples = time_window * systemInfo.sample_rate;
  if(samples >= BUFFER_SIZE) return s_max_speed;
  if(index_GPS < (int)samples - 1) return s_max_speed;

  float sum_kn=0.0f;
  for(uint32_t i=0;i<samples;i++){
    int idx=(index_GPS - samples + 1 + i) % BUFFER_SIZE; if(idx<0) idx += BUFFER_SIZE;
    sum_kn += _sogCms[idx] * CMPS_TO_KNOTS;
  }

  float avg_kn = sum_kn / samples;

  // ========================================================================
  // SESSION BEST (ALL WINDOWS — kept exactly as before)
  // ========================================================================
  if(avg_kn > s_max_speed){
    s_max_speed  = avg_kn;
    avg_speed[9] = s_max_speed;

    int start = index_GPS - (int)samples + 1; if(start < 0) start = 0;

    if(time_window == 2){ win_2s_start=start; win_2s_end=index_GPS; }
    if(time_window == 10){ win_10s_start=start; win_10s_end=index_GPS; }

    getLocalTime(&tmstruct,0);
    time_hour[9]=tmstruct.tm_hour; time_min[9]=tmstruct.tm_min; time_sec[9]=tmstruct.tm_sec;
    this_run[9]=actual_run;

    for(int i=0;i<10;i++) display_speed[i]=avg_speed[i];
    sort_display(display_speed,10);
    display_max_speed = display_speed[9];
  }

  // ========================================================================
  // PER-RUN BEST 10s + TOP-5 WINDOWS (Speedreader style)
  // ========================================================================
  if(time_window == 10 && actual_run > 0 && actual_run < 32){

    // --- update per-run best + its window ---
    if(avg_kn > best_10s_per_run[actual_run]){
      best_10s_per_run[actual_run] = avg_kn;

      int start = index_GPS - (int)samples + 1; if(start < 0) start = 0;
      win_10s_start_run[actual_run] = start;
      win_10s_end_run  [actual_run] = index_GPS;
    }

    // --- recompute avg of best 5 runs (ALWAYS divide by 5) ---
    double tmp[32]; int n=0;
    for(int r=1;r<=run_count;r++) if(best_10s_per_run[r] > 0) tmp[n++] = best_10s_per_run[r];

    if(n > 0){
      sort_display(tmp,n);
      double sum=0.0; int cnt=(n>=5)?5:n;
      for(int i=n-cnt;i<n;i++) sum += tmp[i];
      avg_5runs = (float)(sum / 5.0);   // Speedreader padding rule
    }else{
      avg_5runs = 0.0f;
    }

    // --- recompute TOP-5 run windows (for GeoJSON export) ---
    struct Run10s{ int run; float spd; };
    Run10s runs[32]; int rn=0;

    for(int r=1;r<=run_count;r++){
      if(best_10s_per_run[r] > 0.0f &&
         win_10s_start_run[r] >= 0 &&
         win_10s_end_run[r]   >= win_10s_start_run[r]){
        runs[rn].run = r;
        runs[rn].spd = best_10s_per_run[r];
        rn++;
      }
    }

    // sort ascending by speed
    for(int i=0;i<rn-1;i++)
      for(int j=i+1;j<rn;j++)
        if(runs[i].spd > runs[j].spd){ Run10s t=runs[i]; runs[i]=runs[j]; runs[j]=t; }

    // fill exported arrays (fast, stable, no heap)
    for(int i=0;i<5;i++){ win_10s_top5_start[i]=-1; win_10s_top5_end[i]=-1; }
    win_10s_top5_count = 0;

    for(int i=rn-1;i>=0 && win_10s_top5_count<5;i--){
      int r = runs[i].run;
      int k = win_10s_top5_count++;
      win_10s_top5_start[k] = win_10s_start_run[r];
      win_10s_top5_end  [k] = win_10s_end_run  [r];
    }
  }

  old_run = actual_run;
  return s_max_speed;
}
