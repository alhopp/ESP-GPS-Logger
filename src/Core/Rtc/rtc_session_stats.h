#pragma once

#include <Arduino.h>

void rtc_snapshot_stats();
bool rtc_session_stats_valid();

// Distance (km)
extern RTC_DATA_ATTR float RTC_distance;

// Speeds (knots)
extern RTC_DATA_ATTR float RTC_max_2s_knots;
extern RTC_DATA_ATTR float RTC_avg_10s_knots;
extern RTC_DATA_ATTR float RTC_1h_knots;
extern RTC_DATA_ATTR float RTC_alp_knots;
extern RTC_DATA_ATTR float RTC_mile_knots;

// Records (10s rankings, knots)
extern RTC_DATA_ATTR float RTC_R1_10s;
extern RTC_DATA_ATTR float RTC_R2_10s;
extern RTC_DATA_ATTR float RTC_R3_10s;
extern RTC_DATA_ATTR float RTC_R4_10s;
extern RTC_DATA_ATTR float RTC_R5_10s;
