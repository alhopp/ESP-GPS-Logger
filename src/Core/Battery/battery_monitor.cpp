#include "Core/Battery/battery_monitor.h"

#include <Arduino.h>

#include "Core/battery_config.h"
#include "Core/board_pins.h"
#include "Core/log.h"
#include "Core/Rtc/rtc_battery_state.h"
#include "Config/config_types.h"

namespace {
float raw_battery_adc = 2000.0f;

float calibrationMvPerCount()
{
  return config.cal_bat > 0.0f ? config.cal_bat : BATTERY_ADC_MV_PER_COUNT_DEFAULT;
}

float adcRawToVolts(float rawAdc)
{
  return (rawAdc * calibrationMvPerCount()) / 1000.0f;
}
}

void battery_sample()
{
  // Throw away the first ADC read after boot. The first ESP32 ADC sample can
  // carry stale settling noise after power-up or deep sleep.
  analogRead(BATTERY_ADC_PIN);
  delay(5);

  raw_battery_adc = analogRead(BATTERY_ADC_PIN);
  RTC_voltage_bat = adcRawToVolts(raw_battery_adc);

  LOG_BOOT("Battery", "raw=%.0f cal=%.2f %.2f V",
           raw_battery_adc, calibrationMvPerCount(), RTC_voltage_bat);
}

bool battery_is_low()
{
  return RTC_voltage_bat < RTC_minimum_voltage_bat;
}

float battery_percent(float voltage)
{
  const float percent =
      100.0f * (1.0f - (BATTERY_PERCENT_FULL_V - voltage) /
                            (BATTERY_PERCENT_FULL_V - BATTERY_PERCENT_EMPTY_V));
  return constrain(percent, 0.0f, 100.0f);
}

float battery_display_voltage(float voltage)
{
  return voltage + BATTERY_DISPLAY_OFFSET_V;
}
