#pragma once

#include <Arduino.h>

// ============================================================================
// RTC persistent values (storage)
// ============================================================================

// ---------------------------------------------------------------------------
// UI / layout
// ---------------------------------------------------------------------------

extern RTC_DATA_ATTR char RTC_Sleep_txt[32];
extern RTC_DATA_ATTR int  RTC_Sail_Logo;
extern RTC_DATA_ATTR int  RTC_Board_Logo;

extern RTC_DATA_ATTR int  RTC_SLEEP_screen;
extern RTC_DATA_ATTR int  RTC_OFF_screen;

// ---------------------------------------------------------------------------
// Distance & speed (SI units)
// ---------------------------------------------------------------------------
extern RTC_DATA_ATTR float RTC_distance;
extern RTC_DATA_ATTR float RTC_avg_10s;
extern RTC_DATA_ATTR float RTC_max_2s;
extern RTC_DATA_ATTR float RTC_500m;
extern RTC_DATA_ATTR float RTC_1h;
extern RTC_DATA_ATTR float RTC_mile;
extern RTC_DATA_ATTR float RTC_alp;

// ---------------------------------------------------------------------------
// Distance & speed (knots)
// ---------------------------------------------------------------------------
extern RTC_DATA_ATTR float RTC_avg_10s_knots;
extern RTC_DATA_ATTR float RTC_max_2s_knots;
extern RTC_DATA_ATTR float RTC_alp_knots;
extern RTC_DATA_ATTR float RTC_1h_knots;
extern RTC_DATA_ATTR float RTC_mile_knots;

// ---------------------------------------------------------------------------
// Records (10s rankings)
// ---------------------------------------------------------------------------
extern RTC_DATA_ATTR float RTC_R1_10s;
extern RTC_DATA_ATTR float RTC_R2_10s;
extern RTC_DATA_ATTR float RTC_R3_10s;
extern RTC_DATA_ATTR float RTC_R4_10s;
extern RTC_DATA_ATTR float RTC_R5_10s;

// ---------------------------------------------------------------------------
// Date / time (RTC clock snapshot)
// ---------------------------------------------------------------------------
extern RTC_DATA_ATTR short RTC_year;
extern RTC_DATA_ATTR short RTC_month;
extern RTC_DATA_ATTR short RTC_day;
extern RTC_DATA_ATTR short RTC_hour;
extern RTC_DATA_ATTR short RTC_min;

// ---------------------------------------------------------------------------
// Counters / state
// ---------------------------------------------------------------------------
extern RTC_DATA_ATTR int RTC_counter;

// ---------------------------------------------------------------------------
// Calibration
// ---------------------------------------------------------------------------
extern RTC_DATA_ATTR float calibration_speed;
extern RTC_DATA_ATTR float RTC_calibration_bat;

// ---------------------------------------------------------------------------
// Battery state
// ---------------------------------------------------------------------------
extern RTC_DATA_ATTR float RTC_voltage_bat;
extern RTC_DATA_ATTR float RTC_old_voltage_bat;
extern RTC_DATA_ATTR float RTC_minimum_voltage_bat;

extern RTC_DATA_ATTR int   RTC_bat_choice;
extern RTC_DATA_ATTR int   RTC_highest_read;

// ---------------------------------------------------------------------------
// GPS state (RTC cached)
// ---------------------------------------------------------------------------

extern RTC_DATA_ATTR bool    RTC_gps_valid;
extern RTC_DATA_ATTR uint8_t RTC_gps_baud_index;

