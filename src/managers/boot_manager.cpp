// -----------------------------------------------------------------------------
// boot_manager.cpp
//
// Deterministic early-boot initialisation.
// Responsibilities (report-only):
//  - Bring up Serial with bounded wait
//  - Take a fresh battery ADC sample and scale it
//  - Reset system timebase
//  - Initialise e-paper display early (for error reporting)
//  - Detect fatal boot conditions (low battery / reset boot)
//
// Non-responsibilities:
//  - NO mode transitions
//  - NO sleep / shutdown decisions
//  - NO retries or fallbacks
//
// Returns:
//  - BootResult enum; main.cpp decides what happens next.
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <sys/time.h>
#include "boot_manager.h"

#include "Display/E_paper.h"
#include "task_display.h"

#include "rtc_state.h"
#include "Globals.h"
#include "Definitions.h"


constexpr uint8_t PIN_BAT    = 35;
int BAT_SCALE = 5;

// -----------------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------------
static const char* s_failReason = nullptr;

// -----------------------------------------------------------------------------
// initBoot()
// -----------------------------------------------------------------------------
BootResult initBoot()
{
  s_failReason = nullptr;

// ---- Start Serial Monitor @115200 Baud
  Serial.begin(115200);
  const uint32_t t0 = millis();
  while (millis() - t0 < 400) delay(10);

  LOG_BOOT("Init", "starting");


  // ---- Battery ADC: always take a fresh sample (ignore RTC residue)
  analogRead(PIN_BAT);
  delay(5);              
  analog_mean       = analogRead(PIN_BAT);
  RTC_voltage_bat   = analog_mean * BAT_SCALE;

  LOG_BOOT("Battery", "%.2f V", RTC_voltage_bat);

  // ---- Timebase: reset to epoch (RTC validity determined later)
  timeval tv = {}; 
  settimeofday(&tv, nullptr);

  // ---- Display: early init for deterministic error reporting
  LOG_BOOT("Display", "init");
  display.init(115200, true, 2, false);
  display.setRotation(1);
  display.setTextColor(GxEPD_BLACK);


  // ---- Fatal boot conditions (report only)
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




// -----------------------------------------------------------------------------
// bootFailReason()
// -----------------------------------------------------------------------------
const char* bootFailReason() { return s_failReason; }
