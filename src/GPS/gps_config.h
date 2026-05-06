#pragma once

// GPS policy thresholds and unit conversions.
// These are application-level GPS rules, not u-blox protocol definitions.

// First-fix gate used before a logging session can start.
constexpr int MIN_numSV_FIRST_FIX = 5;
constexpr int MAX_Sacc_FIRST_FIX = 2;

// Per-sample quality gate used before a sample contributes to statistics.
constexpr int MIN_numSV_GPS_SPEED_OK = 4;
constexpr int MAX_Sacc_GPS_SPEED_OK = 1;
constexpr int MAX_GPS_SPEED_OK = 40;   // m/s

// Run detector timing.
constexpr int TIME_DELAY_NEW_RUN = 10;

// Unit helpers.
constexpr double MMPS_TO_KNOTS = 0.0019438444924406;
constexpr double CMPS_TO_KNOTS = 0.019438444924406;
