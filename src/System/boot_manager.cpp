// ============================================================================
// boot_manager.cpp
//
// Deterministic early-boot initialisation.
// This module reports whether boot may continue; it does not change system mode.
// ============================================================================

#include "System/boot_manager.h"

#include <Arduino.h>
#include <sys/time.h>

#include "Display/E_paper.h"
#include "Core/Battery/battery_monitor.h"
#include "Core/log.h"

namespace {
constexpr uint32_t SERIAL_WAIT_MS = 400;

void initSerial()
{
  Serial.begin(115200);

  const uint32_t t0 = millis();
  while (millis() - t0 < SERIAL_WAIT_MS) {
    delay(10);
  }

  LOG_BOOT("Init", "starting");
}

void resetTimebase()
{
  // Start from a deterministic zero timebase. GPS/NTP code sets real time later
  // after valid data is available.
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
  if (battery_is_low()) {
    LOG_BOOT("Shutdown", "low battery");
    return BOOT_LOW_BATTERY;
  }

  return BOOT_OK;
}
}

BootResult initBoot()
{
  initSerial();
  battery_sample();
  resetTimebase();
  initEarlyDisplay();

  const BootResult result = checkFatalBootConditions();
  if (result != BOOT_OK) {
    return result;
  }

  LOG_BOOT("Status", "boot checks passed");
  return BOOT_OK;
}
