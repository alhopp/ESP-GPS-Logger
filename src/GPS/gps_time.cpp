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
// SBP-aligned time-window speed statistics.
//
// UNIT MODEL
// - Raw input        : cm/s (_sogCms[], _secSpeed[])
// - Per-sample       : convert to knots FIRST
// - Averaging        : FLOAT knots (SBP-exact)
// - Storage / ranking: knots
//
// WINDOWS
// - 2s   = 2  * sample_rate (5 Hz)
// - 10s  = 10 * sample_rate (5 Hz)
// - 1h   = 3600 samples (1 Hz, padded)
// ============================================================================

// ----------------------------------------------------------------------------
GPS_time::GPS_time(int tijdvenster):time_window(tijdvenster){ Reset_stats(); }

// ----------------------------------------------------------------------------
void GPS_time::Reset_stats(){
  for(int i=0;i<10;i++){ avg_speed[i]=0; display_speed[i]=0; }
  avg_5runs=0; avg_s_sum=0; s_max_speed=0;
}



// ----------------------------------------------------------------------------
float GPS_time::Update_speed(int actual_run)
{
  // ========================================================================
  // 1 HOUR (3600 s) — 1 Hz data, padded (missing seconds = zero)
  // ========================================================================
  if(time_window==3600){
    int secs=index_sec+1; if(secs<=0) return s_max_speed; if(secs>3600) secs=3600;

    float sum_kn=0;
    for(int i=0;i<secs;i++){
      int idx=(index_sec-i)%BUFFER_SIZE; if(idx<0) idx+=BUFFER_SIZE;
      sum_kn+=_secSpeed[idx]*CMPS_TO_KNOTS;        // cm/s → knots (per sample)
    }

    float avg_kn=(sum_kn/secs)*((float)secs/3600.0f); // padding
    if(avg_kn>s_max_speed) s_max_speed=avg_kn;
    return s_max_speed;
  }

  // ========================================================================
  // 2s / 10s — SBP-style, 5 Hz samples
  // ========================================================================
  if(time_window*systemInfo.sample_rate>=BUFFER_SIZE) return s_max_speed;

  const uint32_t samples=time_window*systemInfo.sample_rate;
  if(index_GPS<(int)samples-1) return s_max_speed;   // window not full

  // Per-sample cm/s → knots, then average
  float sum_kn=0;
  for(uint32_t i=0;i<samples;i++){
    int idx=(index_GPS-samples+1+i)%BUFFER_SIZE; if(idx<0) idx+=BUFFER_SIZE;
    sum_kn+=_sogCms[idx]*CMPS_TO_KNOTS;
  }
  float avg_kn=sum_kn/samples;

  // ========================================================================
  // NEW MAX DETECTED
  // ========================================================================
  if(avg_kn>s_max_speed){

    s_max_speed=avg_kn; avg_speed[0]=s_max_speed;
    if(avg_speed[9]<s_max_speed) avg_speed[9]=s_max_speed;

    speed_run[actual_run%NR_OF_BAR]=s_max_speed;

    getLocalTime(&tmstruct,0);
    time_hour[0]=tmstruct.tm_hour; time_min[0]=tmstruct.tm_min;
    time_sec[0]=tmstruct.tm_sec;   this_run[0]=actual_run;

    for(int i=0;i<10;i++) display_speed[i]=avg_speed[i];
    sort_display(display_speed,10);

    // Progressive avg of best 5 × 10s (zeros included)
    if(time_window==10){
      float sum5=0; for(int i=5;i<10;i++) sum5+=avg_speed[i];
      avg_5runs=sum5/5.0f;
    }

    display_max_speed=display_speed[9];
  }

  old_run=actual_run;
  return s_max_speed;
}
