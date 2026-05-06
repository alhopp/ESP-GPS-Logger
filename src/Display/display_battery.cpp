#include "Display/display_battery.h"

#include "Core/Battery/battery_monitor.h"

float displayBatteryPercent(float voltage)
{
  return battery_percent(voltage);
}
