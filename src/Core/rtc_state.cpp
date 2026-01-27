#include "Core/rtc_state.h"
#include "Core/Definitions.h"

#include "GPS/GPS_data.h"


// ============================================================================
// RTC persistent values (storage)
// ============================================================================


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
// Records (10s rankings)
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
RTC_DATA_ATTR uint8_t RTC_gps_baud_index  = 0;   // index into baud table



void rtc_snapshot_stats()
{
  // -------------------------------------------------------------------------
  // Speeds (mm/s → knots)
  // -------------------------------------------------------------------------
  RTC_max_2s_knots  = S2.avg_speed[9] * MMPS_TO_KNOTS;  // BEST 2sec of entire session
  RTC_avg_10s_knots = S10.avg_5runs  * MMPS_TO_KNOTS;

  // NM = speed over 1852 m window (NOT distance)
  RTC_mile_knots   = M1852.m_speed        * MMPS_TO_KNOTS;
  RTC_alp_knots    = A500.alfa_speed_max  * MMPS_TO_KNOTS;
  RTC_1h_knots     = S3600.s_max_speed * MMPS_TO_KNOTS;


  // -------------------------------------------------------------------------
  // Ranked 10s speeds (knots)
  // -------------------------------------------------------------------------
  RTC_R1_10s = S10.avg_speed[9] * MMPS_TO_KNOTS;
  RTC_R2_10s = S10.avg_speed[8] * MMPS_TO_KNOTS;
  RTC_R3_10s = S10.avg_speed[7] * MMPS_TO_KNOTS;
  RTC_R4_10s = S10.avg_speed[6] * MMPS_TO_KNOTS;
  RTC_R5_10s = S10.avg_speed[5] * MMPS_TO_KNOTS;

  // -------------------------------------------------------------------------
  // Distances (travelled)
  // total_distance is mm
  // -------------------------------------------------------------------------

  // ----- distance (meters) -----
  RTC_distance      = total_distance / 1000.0f;  // mm → meters
}
