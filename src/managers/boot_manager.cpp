// -----------------------------------------------------------------------------
// boot_manager.cpp
//
// Early boot sequence:
// - Starts Serial with bounded wait
// - Reads battery voltage
// - Initializes SPI and resets system time
// - Initializes the e-paper display
// - Enforces shutdown on low battery or reset boot
//
// Runs once at startup.
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <SPI.h>
#include <sys/time.h>

#include "boot_manager.h"
#include "ESP_functions.h"
#include "config_manager.h"
#include "E_paper.h"
#include "rtc_state.h"
#include "system_mode.h"

#include "Globals.h"


// Logging macros live here
#include "Definitions.h"

// Fonts (e-paper friendly)
#include "Fonts.h"

// -----------------------------------------------------------------------------
// EXTERNAL / RTC STATE
// -----------------------------------------------------------------------------
bool reset_boot;

// Battery scaling (must match hardware divider)
#ifndef BAT_SCALE
#define BAT_SCALE 1.0f
#endif

// -----------------------------------------------------------------------------
// LOCAL HELPERS (file-scope only)
// -----------------------------------------------------------------------------
static void shutdownWithMessage(const char* msg)
{
  RTC_OFF_screen = 1;

  strncpy(RTC_Sleep_txt, msg, sizeof(RTC_Sleep_txt) - 1);
  RTC_Sleep_txt[sizeof(RTC_Sleep_txt) - 1] = '\0';

  setMode(MODE_SLEEP);    // does not return
}

// -----------------------------------------------------------------------------
// BOOT ENTRY POINT
// -----------------------------------------------------------------------------
void initBoot()
{
  // ---------------------------------------------------------------------------
  // Serial (ESP32-correct, bounded, deterministic)
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
  // Display init (no dependencies on storage or Wi-Fi)
  // ---------------------------------------------------------------------------
  LOG_BOOT("Display", "init");
  display.init(115200, true, 2, false);
  display.setRotation(1);

  // Hold splash ONLY on true cold boot
  if (!reset_boot) {
    delay(1200);
  }

  // ---------------------------------------------------------------------------
  // Safety exits (hard stops)
  // ---------------------------------------------------------------------------
  if (RTC_voltage_bat < RTC_minimum_voltage_bat) {
    LOG_BOOT("Shutdown", "low battery");
    shutdownWithMessage("Shut down Low Bat!");
    return;
  }

  if (reset_boot) {
    LOG_BOOT("Shutdown", "after reset");
    shutdownWithMessage("Shutdown after reset!");
    return;
  }

  LOG_BOOT("Status", "boot checks passed");
}
