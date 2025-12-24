#include "rtc_state.h"
#include "Definitions.h"

// ============================================================================
// RTC persistent values (storage)
// ============================================================================

// ---------------------------------------------------------------------------
// UI / layout
// ---------------------------------------------------------------------------
RTC_DATA_ATTR int RTC_offset = 0;   // main UI offset
RTC_DATA_ATTR int offset     = 0;   // legacy alias still used in code

RTC_DATA_ATTR char RTC_Sleep_txt[32] = "Your ID";
RTC_DATA_ATTR int  RTC_Sail_Logo     = 0;
RTC_DATA_ATTR int  RTC_Board_Logo    = 0;

RTC_DATA_ATTR int  RTC_SLEEP_screen  = 0;
RTC_DATA_ATTR int  RTC_OFF_screen    = 0;

// ---------------------------------------------------------------------------
// Distance & speed (SI units)
// ---------------------------------------------------------------------------
RTC_DATA_ATTR float RTC_distance = 0.0f;
RTC_DATA_ATTR float RTC_avg_10s  = 0.0f;
RTC_DATA_ATTR float RTC_max_2s   = 0.0f;
RTC_DATA_ATTR float RTC_500m     = 0.0f;
RTC_DATA_ATTR float RTC_1h       = 0.0f;
RTC_DATA_ATTR float RTC_mile     = 0.0f;
RTC_DATA_ATTR float RTC_alp      = 0.0f;

// ---------------------------------------------------------------------------
// Distance & speed (knots)
// ---------------------------------------------------------------------------
RTC_DATA_ATTR float RTC_avg_10s_knots = 0.0f;
RTC_DATA_ATTR float RTC_max_2s_knots  = 0.0f;
RTC_DATA_ATTR float RTC_alp_knots     = 0.0f;
RTC_DATA_ATTR float RTC_1h_knots      = 0.0f;
RTC_DATA_ATTR float RTC_mile_knots    = 0.0f;

// ---------------------------------------------------------------------------
// Records (10s rankings)
// ---------------------------------------------------------------------------
RTC_DATA_ATTR float RTC_R1_10s = 0.0f;
RTC_DATA_ATTR float RTC_R2_10s = 0.0f;
RTC_DATA_ATTR float RTC_R3_10s = 0.0f;
RTC_DATA_ATTR float RTC_R4_10s = 0.0f;
RTC_DATA_ATTR float RTC_R5_10s = 0.0f;

// ---------------------------------------------------------------------------
// Date / time (RTC clock snapshot)
// ---------------------------------------------------------------------------
RTC_DATA_ATTR short RTC_year  = 0;
RTC_DATA_ATTR short RTC_month = 0;
RTC_DATA_ATTR short RTC_day   = 0;
RTC_DATA_ATTR short RTC_hour  = 0;
RTC_DATA_ATTR short RTC_min   = 0;

// ---------------------------------------------------------------------------
// Counters / state
// ---------------------------------------------------------------------------
RTC_DATA_ATTR int RTC_counter = 0;

// ---------------------------------------------------------------------------
// Calibration
// ---------------------------------------------------------------------------
RTC_DATA_ATTR float calibration_speed   = 3.6f;   // km/h default
RTC_DATA_ATTR float RTC_calibration_bat = 1.75f;

// ---------------------------------------------------------------------------
// Battery state
// ---------------------------------------------------------------------------
RTC_DATA_ATTR float RTC_voltage_bat         = 3.6f;
RTC_DATA_ATTR float RTC_old_voltage_bat     = 3.6f;
RTC_DATA_ATTR float RTC_minimum_voltage_bat = MINIMUM_VOLTAGE;

RTC_DATA_ATTR int   RTC_bat_choice   = 0;
RTC_DATA_ATTR int   RTC_highest_read = STARTVALUE_HIGHEST_READ;
