// -----------------------------------------------------------------------------
// boot_manager.cpp
//
// Deterministic early-boot initialisation.
// This module reports whether boot may continue; it does not change system mode.
// -----------------------------------------------------------------------------

#include "System/boot_manager.h"

#include <Arduino.h>
#include <sys/time.h>

#include "Display/E_paper.h"
#include "Core/battery_config.h"
#include "Core/board_pins.h"
#include "Core/log.h"
#include "Core/Globals.h"
#include "Core/rtc_state.h"

namespace {
constexpr float BAT_SCALE = 5.0f;
constexpr uint32_t SERIAL_WAIT_MS = 400;

const char* s_failReason = nullptr;

void initSerial()
{
  Serial.begin(115200);

  const uint32_t t0 = millis();
  while (millis() - t0 < SERIAL_WAIT_MS) {
    delay(10);
  }

  LOG_BOOT("Init", "starting");
}

void sampleBattery()
{
  analogRead(BATTERY_ADC_PIN);
  delay(5);

  analog_mean = analogRead(BATTERY_ADC_PIN);
  RTC_voltage_bat = analog_mean * BAT_SCALE;

  LOG_BOOT("Battery", "%.2f V", RTC_voltage_bat);
}

void resetTimebase()
{
  timeval tv = {};
  settimeofday(&tv, nullptr);
}

void initEarlyDisplay()
{
  LOG_BOOT("Display", "init");
  display_init();
}

BootResult checkFatalBootConditions()
{
  if (RTC_voltage_bat < RTC_minimum_voltage_bat) {
    LOG_BOOT("Shutdown", "low battery");
    s_failReason = "Shut down Low Bat!";
    return BOOT_LOW_BATTERY;
  }

  if (reset_boot) {
    LOG_BOOT("Shutdown", "after reset");
    s_failReason = "Shutdown after reset!";
    return BOOT_AFTER_RESET;
  }

  return BOOT_OK;
}
}

BootResult initBoot()
{
  s_failReason = nullptr;

  initSerial();
  sampleBattery();
  resetTimebase();
  initEarlyDisplay();

  const BootResult result = checkFatalBootConditions();
  if (result != BOOT_OK) {
    return result;
  }

  LOG_BOOT("Status", "boot checks passed");
  return BOOT_OK;
}

const char* bootFailReason()
{
  return s_failReason;
}
