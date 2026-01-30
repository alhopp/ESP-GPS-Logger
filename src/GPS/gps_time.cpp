#include "GPS/gps_time.h"
#include "GPS/GPS_data.h"
#include "GPS/gps_utils.h"

#include "Ublox/ublox.h"
#include "Core/Globals.h"
#include "core/system_info.h"

#include <time.h>

// ============================================================================
// GPS_time
// SBP-aligned speed statistics
//
// Internal storage : cm/s
// Display / ranking : knots (float)
// Averaging         : FLOAT knots (SBP-exact)
//
// 2s  = 2 * sample_rate samples
// 10s = 10 * sample_rate samples
// ============================================================================

// ----------------------------------------------------------------------------
GPS_time::GPS_time(int tijdvenster) : time_window(tijdvenster){
  Reset_stats();
}

// ----------------------------------------------------------------------------
void GPS_time::Reset_stats(){
  for(int i=0;i<10;i++){
    avg_speed[i]=0;
    display_speed[i]=0;
  }
  avg_5runs     = 0;
  avg_s_sum     = 0;
  s_max_speed   = 0;
}

// ----------------------------------------------------------------------------
// Debug helper — prints EXACT contributing samples
// ----------------------------------------------------------------------------
static void dump_window(const char *label, uint32_t samples){
  Serial.printf("\n=== NEW BEST %s WINDOW (SBP) ===\n", label);
  Serial.printf("index_GPS = %d\n", index_GPS);
  Serial.printf("samples   = %lu\n", samples);

  float sum_kn = 0.0f;

  for(uint32_t i=0;i<samples;i++){
    int idx = (index_GPS - samples + 1 + i) % BUFFER_SIZE;
    if(idx < 0) idx += BUFFER_SIZE;

    uint16_t cmps = _sogCms[idx];
    float kn = cmps * CMPS_TO_KNOTS;
    sum_kn += kn;

    Serial.printf(
      "  [%3lu] sogCms[%d] = %4u cm/s (%.3f kn)\n",
      i, idx, cmps, kn
    );
  }

  Serial.printf("AVG = %.3f kn\n", sum_kn / samples);
  Serial.println("========================================\n");
}

// ----------------------------------------------------------------------------
float GPS_time::Update_speed(int actual_run)
{
  // --------------------------------------------------------------------------
  // FAST PATH — 2s / 10s (SBP-style)
  // --------------------------------------------------------------------------
  if(time_window * systemInfo.sample_rate >= BUFFER_SIZE)
    return s_max_speed;

  const uint32_t samples = time_window * systemInfo.sample_rate;

  // window not yet full
  if(index_GPS < (int)samples - 1)
    return s_max_speed;

  // --------------------------------------------------------------------------
  // SBP-EXACT averaging: average FLOAT knots
  // --------------------------------------------------------------------------
  float sum_kn = 0.0f;

  for(uint32_t i=0;i<samples;i++){
    int idx = (index_GPS - samples + 1 + i) % BUFFER_SIZE;
    if(idx < 0) idx += BUFFER_SIZE;

    sum_kn += _sogCms[idx] * CMPS_TO_KNOTS;
  }

  float avg_kn = sum_kn / samples;

  // --------------------------------------------------------------------------
  // NEW MAX DETECTED
  // --------------------------------------------------------------------------
  if(avg_kn > s_max_speed){

    if(time_window == 2)
      dump_window("2s", samples);
    else if(time_window == 10)
      dump_window("10s", samples);

    s_max_speed   = avg_kn;
    avg_speed[0]  = s_max_speed;

    // immediate session-best promotion
    if(avg_speed[9] < s_max_speed)
      avg_speed[9] = s_max_speed;

    speed_run[actual_run % NR_OF_BAR] = s_max_speed;

    getLocalTime(&tmstruct,0);
    time_hour[0]=tmstruct.tm_hour;
    time_min [0]=tmstruct.tm_min;
    time_sec [0]=tmstruct.tm_sec;
    this_run[0]=actual_run;

    for(int i=0;i<10;i++)
      display_speed[i]=avg_speed[i];

    sort_display(display_speed,10);
    display_max_speed = display_speed[9];
  }

  old_run = actual_run;
  return s_max_speed;
}
