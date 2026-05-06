#include "Display/display_battery.h"

#include <Arduino.h>

#include "Core/battery_config.h"

float displayBatteryPercent(float voltage)
{
  const float percent = 100.0f * (1.0f - (VOLTAGE_100 - voltage) / (VOLTAGE_100 - VOLTAGE_0));
  return constrain(percent, 0.0f, 100.0f);
}
