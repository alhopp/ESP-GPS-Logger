#pragma once

// ============================================================================
// battery_monitor.h
//
// Battery measurement and derived battery display values.
//
// This module owns ADC sampling and conversion into RTC_voltage_bat. The
// conversion follows the RP6-style `cal_bat` calibration value:
// raw ADC count * millivolts-per-count / 1000 = volts.
// ============================================================================

// Sample the battery ADC and update RTC_voltage_bat.
void battery_sample();

// True when the last sampled voltage is below the configured shutdown voltage.
bool battery_is_low();

// Convert a battery voltage to a display percentage.
float battery_percent(float voltage);

// Apply the small RP6 display-only voltage offset used in the footer.
float battery_display_voltage(float voltage);
