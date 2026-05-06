#pragma once

// Battery voltage calibration and display thresholds.

constexpr float CALIBRATION_BAT_V = 1.7f;

constexpr float VOLTAGE_100 = 4.15f;
constexpr float VOLTAGE_0 = 3.4f;

constexpr float MINIMUM_VOLTAGE = 0.0f;
constexpr float MINIMUM_VOLTAGE_CHANGE = 0.1f;

constexpr int TOLERANCE = 100;
constexpr int VOLTAGE_LOW = 25;