#include "Core/rtc_state.h"
#include "Core/Definitions.h"

#include "GPS/GPS_data.h"
#include "GPS/gps_speed.h"
#include "GPS/gps_alpha.h"
#include "GPS/gps_time.h"

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


// ---------------------------------------------------------------------------
// Snapshot all performance stats to RTC + Serial
// ---------------------------------------------------------------------------
void rtc_snapshot_stats()
{
  Serial.println("\n================ RTC SNAPSHOT STATS ================");

  // -------------------------------------------------------------------------
  // 2 second (session best)
  // -------------------------------------------------------------------------
  RTC_max_2s_knots = S2.s_max_speed;
  Serial.printf("2s max          : %.3f kn\n", RTC_max_2s_knots);

  // -------------------------------------------------------------------------
  // Per-run 10s + average of best 5 runs
  // -------------------------------------------------------------------------
  float sum5 = 0.0f;
  int   used = 0;

  Serial.println("10s best per run:");

  if(S10.run_count >= 1){ RTC_R1_10s = S10.best_10s_per_run[1]; Serial.printf("  R1            : %.3f kn\n", RTC_R1_10s); sum5 += RTC_R1_10s; used++; }
  if(S10.run_count >= 2){ RTC_R2_10s = S10.best_10s_per_run[2]; Serial.printf("  R2            : %.3f kn\n", RTC_R2_10s); sum5 += RTC_R2_10s; used++; }
  if(S10.run_count >= 3){ RTC_R3_10s = S10.best_10s_per_run[3]; Serial.printf("  R3            : %.3f kn\n", RTC_R3_10s); sum5 += RTC_R3_10s; used++; }
  if(S10.run_count >= 4){ RTC_R4_10s = S10.best_10s_per_run[4]; Serial.printf("  R4            : %.3f kn\n", RTC_R4_10s); sum5 += RTC_R4_10s; used++; }
  if(S10.run_count >= 5){ RTC_R5_10s = S10.best_10s_per_run[5]; Serial.printf("  R5            : %.3f kn\n", RTC_R5_10s); sum5 += RTC_R5_10s; used++; }

  RTC_avg_10s_knots = (used > 0) ? (sum5 / used) : 0.0f;
  Serial.printf("10s avg (best %d): %.3f kn\n", used, RTC_avg_10s_knots);

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
