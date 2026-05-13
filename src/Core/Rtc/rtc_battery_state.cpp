#include "Core/Rtc/rtc_battery_state.h"

#include "Core/battery_config.h"

RTC_DATA_ATTR float RTC_voltage_bat = 3.6f;
RTC_DATA_ATTR float RTC_minimum_voltage_bat = BATTERY_SHUTDOWN_VOLTAGE_DEFAULT;
