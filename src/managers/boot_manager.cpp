// -----------------------------------------------------------------------------
// boot_manager.cpp
//
// Early boot sequence:
// - Starts Serial with bounded wait
// - Reads battery voltage
// - Resets system time
// - Initializes the e-paper display
// - Reports fatal boot conditions (low battery / reset boot)
//
// NOTE:
// - This module performs NO mode transitions.
// - It reports BootResult; main.cpp decides what to do.
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <SPI.h>
#include <sys/time.h>

#include "boot_manager.h"
#include "E_paper.h"
#include "rtc_state.h"
#include "Globals.h"
#include "Definitions.h"

// -----------------------------------------------------------------------------
// INTERNAL STATE
// -----------------------------------------------------------------------------
static const char* s_failReason = nullptr;

// Battery scaling (must match hardware divider)
#ifndef BAT_SCALE
#define BAT_SCALE 1.0f
#endif

// -----------------------------------------------------------------------------
// PUBLIC API
// -----------------------------------------------------------------------------
BootResult initBoot()
{
  s_failReason = nullptr;

  // ---------------------------------------------------------------------------
  // Serial (bounded, deterministic)
  // ---------------------------------------------------------------------------
  Serial.begin(115200);

  const uint32_t t0 = millis();
  while (millis() - t0 < 400) {
    delay(10);
  }

  LOG_BOOT("Init", "starting");

  // ---------------------------------------------------------------------------
  // Battery ADC (fresh sample – never trust stale RTC data)
  // ---------------------------------------------------------------------------
  analogRead(PIN_BAT);   // discard first read
  delay(5);
  analog_mean = analogRead(PIN_BAT);

  RTC_voltage_bat = analog_mean * BAT_SCALE;

  LOG_BOOT("Battery", "%.2f V", RTC_voltage_bat);

  // ---------------------------------------------------------------------------
  // Timebase
  // ---------------------------------------------------------------------------
  struct timeval tv = {};
  settimeofday(&tv, nullptr);

  // ---------------------------------------------------------------------------
  // Display init (early, deterministic)
  // ---------------------------------------------------------------------------
  LOG_BOOT("Display", "init");
  display.init(115200, true, 2, false);
  display.setRotation(1);
  display.setTextColor(GxEPD_BLACK);
  
  // Hold splash ONLY on true cold boot
  if (!reset_boot) {
    delay(1200);
  }

  // ---------------------------------------------------------------------------
  // Fatal boot conditions (report only)
  // ---------------------------------------------------------------------------
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

  LOG_BOOT("Status", "boot checks passed");
  return BOOT_OK;

}


const char* bootFailReason()
{
  return s_failReason;
}
