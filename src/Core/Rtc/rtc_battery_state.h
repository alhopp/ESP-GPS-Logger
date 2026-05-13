#pragma once

// ============================================================================
// RTC battery state
//
// Battery readings retained across deep sleep so boot/display code can show the
// last sampled voltage and enforce the configured shutdown threshold.
// ============================================================================

#include <Arduino.h>

extern RTC_DATA_ATTR float RTC_voltage_bat;
extern RTC_DATA_ATTR float RTC_minimum_voltage_bat;
