#pragma once

// Battery voltage calibration and display thresholds.
//
// RP6 uses a millivolts-per-ADC-count calibration value. The web-configurable
// `config.cal_bat` follows the same model:
//
//   battery volts = raw ADC count * cal_bat / 1000
//
// A value around 1.7 means an ADC reading around 2400 maps to about 4.1 V.

constexpr float BATTERY_ADC_MV_PER_COUNT_DEFAULT = 1.7f;
constexpr float BATTERY_DISPLAY_OFFSET_V = 0.04f;

constexpr float BATTERY_PERCENT_FULL_V = 4.15f;
constexpr float BATTERY_PERCENT_EMPTY_V = 3.4f;
constexpr float BATTERY_SHUTDOWN_VOLTAGE_DEFAULT = 3.2f;

// Compatibility aliases for older modules/config defaults.
constexpr float VOLTAGE_100 = BATTERY_PERCENT_FULL_V;
constexpr float VOLTAGE_0 = BATTERY_PERCENT_EMPTY_V;
constexpr float MINIMUM_VOLTAGE = BATTERY_SHUTDOWN_VOLTAGE_DEFAULT;
