#include "Core/Rtc/rtc_battery_state.h"

// ============================================================================
// RTC battery state
//
// Definitions for battery values retained in RTC memory across deep sleep.
// ============================================================================

#include "Core/Battery/battery_config.h"

RTC_DATA_ATTR float RTC_voltage_bat = 3.6f;
RTC_DATA_ATTR float RTC_minimum_voltage_bat = BATTERY_SHUTDOWN_VOLTAGE_DEFAULT;
