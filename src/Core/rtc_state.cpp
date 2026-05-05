#include "Core/rtc_state.h"
#include "Core/Definitions.h"

#include "GPS/Data/gps_data.h"
#include "GPS/Metrics/gps_distance_speed.h"
#include "GPS/Metrics/gps_alpha_speed.h"
#include "GPS/Metrics/gps_time_speed.h"
#include "GPS/Metrics/gps_run_detector.h"
#include "GPS/Metrics/gps_result_sort.h"

// ---------------------------------------------------------------------------
// Distance & speed (SI units)
// ---------------------------------------------------------------------------
RTC_DATA_ATTR float RTC_distance      = 0.0f;
RTC_DATA_ATTR float RTC_avg_10s       = 0.0f;
RTC_DATA_ATTR float RTC_max_2s        = 0.0f;
RTC_DATA_ATTR float RTC_500m          = 0.0f;
RTC_DATA_ATTR float RTC_1h            = 0.0f;
RTC_DATA_ATTR float RTC_alp           = 0.0f;
RTC_DATA_ATTR float RTC_nm_knots      = 0.0f;

// ---------------------------------------------------------------------------
// Distance & speed (knots)
// ---------------------------------------------------------------------------
RTC_DATA_ATTR float RTC_avg_10s_knots  = 0.0f;
RTC_DATA_ATTR float RTC_max_2s_knots   = 0.0f;
RTC_DATA_ATTR float RTC_alp_knots      = 0.0f;
RTC_DATA_ATTR float RTC_1h_knots       = 0.0f;
RTC_DATA_ATTR float RTC_mile_knots     = 0.0f;

// ---------------------------------------------------------------------------
// Records (per-run 10s)
// ---------------------------------------------------------------------------
RTC_DATA_ATTR float RTC_R1_10s         = 0.0f;
RTC_DATA_ATTR float RTC_R2_10s         = 0.0f;
RTC_DATA_ATTR float RTC_R3_10s         = 0.0f;
RTC_DATA_ATTR float RTC_R4_10s         = 0.0f;
RTC_DATA_ATTR float RTC_R5_10s         = 0.0f;

// ---------------------------------------------------------------------------
// Date / time (RTC clock snapshot)
// ---------------------------------------------------------------------------
RTC_DATA_ATTR short RTC_year           = 0;
RTC_DATA_ATTR short RTC_month          = 0;
RTC_DATA_ATTR short RTC_day            = 0;
RTC_DATA_ATTR short RTC_hour           = 0;
RTC_DATA_ATTR short RTC_min            = 0;

// ---------------------------------------------------------------------------
// Counters / state
// ---------------------------------------------------------------------------
RTC_DATA_ATTR int   RTC_counter        = 0;

// ---------------------------------------------------------------------------
// Calibration
// ---------------------------------------------------------------------------
RTC_DATA_ATTR float RTC_calibration_bat = 1.75f;

// ---------------------------------------------------------------------------
// Battery state
// ---------------------------------------------------------------------------
RTC_DATA_ATTR float RTC_voltage_bat          = 3.6f;
RTC_DATA_ATTR float RTC_old_voltage_bat      = 3.6f;
RTC_DATA_ATTR float RTC_minimum_voltage_bat  = MINIMUM_VOLTAGE;

RTC_DATA_ATTR int   RTC_highest_read          = STARTVALUE_HIGHEST_READ;

// ---------------------------------------------------------------------------
// GPS state (RTC cached)
// ---------------------------------------------------------------------------
RTC_DATA_ATTR bool    RTC_gps_valid       = false;
RTC_DATA_ATTR uint8_t RTC_gps_baud_index  = 0;

static int last_printed_run = -1;


// ---------------------------------------------------------------------------
// Snapshot all performance stats to RTC + Serial
// ---------------------------------------------------------------------------
void rtc_snapshot_stats()
{
  Serial.println("\n================ RTC SNAPSHOT STATS ================");

  // -------------------------------------------------------------------------
  // DEBUG: Run start correlation (Speedreader alignment)
  // -------------------------------------------------------------------------
  const int current_run = gps_run_current();
  if(current_run != last_printed_run && gps_run_started()){
    last_printed_run = current_run;
    Serial.printf(
      "[RUN ] #%d started at GPS sample %d\n",
      current_run,
      index_GPS
    );
  }

  // -------------------------------------------------------------------------
  // 2 second (session best)
  // -------------------------------------------------------------------------
  RTC_max_2s_knots = S2.s_max_speed;
  Serial.printf("2s max          : %.3f kn\n", RTC_max_2s_knots);

  // -------------------------------------------------------------------------
  // Per-run 10s — TOP 5 (Speedreader semantics)
  // -------------------------------------------------------------------------
  double tmp[32];
  int    n = 0;

  // Collect all valid per-run bests
  for(int r = 1; r <= S10.run_count && r < 32; r++){
    if(S10.best_10s_per_run[r] > 0)
      tmp[n++] = S10.best_10s_per_run[r];
  }

  // Sort ascending
  if(n > 1)
    sort_display(tmp, n);

  // Clear RTC slots
  RTC_R1_10s = RTC_R2_10s = RTC_R3_10s = RTC_R4_10s = RTC_R5_10s = 0.0f;

  // Take top 5 (highest values)
  int cnt = (n >= 5) ? 5 : n;
  double sum5 = 0;

  Serial.println("10s best (top 5 runs):");

  for(int i = 0; i < cnt; i++){
    const float v = tmp[n - 1 - i];
    sum5 += v;

    switch(i){
      case 0: RTC_R1_10s = v; Serial.printf("  #1            : %.3f kn\n", v); break;
      case 1: RTC_R2_10s = v; Serial.printf("  #2            : %.3f kn\n", v); break;
      case 2: RTC_R3_10s = v; Serial.printf("  #3            : %.3f kn\n", v); break;
      case 3: RTC_R4_10s = v; Serial.printf("  #4            : %.3f kn\n", v); break;
      case 4: RTC_R5_10s = v; Serial.printf("  #5            : %.3f kn\n", v); break;
    }
  }

  // Average of best 5
  RTC_avg_10s_knots = sum5 / 5.0f;
  Serial.printf("10s avg (best %d): %.3f kn\n", cnt, RTC_avg_10s_knots);


  // -------------------------------------------------------------------------
  // Mile / Alpha / 1 hour
  // -------------------------------------------------------------------------
  RTC_mile_knots = M1852.avg_speed[9];
  RTC_alp_knots  = A500.avg_speed[9];
  RTC_1h_knots   = S3600.s_max_speed;

  Serial.printf("NM (1852m)      : %.3f kn\n", RTC_mile_knots);
  Serial.printf("Alpha 500       : %.3f kn\n", RTC_alp_knots);
  Serial.printf("1 hour          : %.3f kn\n", RTC_1h_knots);

  // -------------------------------------------------------------------------
  // Distance travelled
  // -------------------------------------------------------------------------
  RTC_distance = total_distance * 0.00001f;   // cm → km
  Serial.printf("Distance        : %.3f km\n", RTC_distance);

  Serial.println("====================================================\n");
}
