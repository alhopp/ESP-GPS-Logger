#pragma once

// ============================================================================
// RTC time state
//
// Compact date/time fields retained across deep sleep for screens that need the
// previous known time before GPS time has been accepted again.
// ============================================================================

#include <Arduino.h>

// Date / time snapshot cached across deep sleep.
extern RTC_DATA_ATTR short RTC_year;
extern RTC_DATA_ATTR short RTC_month;
extern RTC_DATA_ATTR short RTC_day;
extern RTC_DATA_ATTR short RTC_hour;
extern RTC_DATA_ATTR short RTC_min;
