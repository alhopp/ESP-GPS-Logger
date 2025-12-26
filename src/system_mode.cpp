// -----------------------------------------------------------------------------
// system_mode.cpp
//
// Central system mode state machine.
//
// Responsibilities:
// - Own the current SystemMode
// - Perform EXIT actions for the old mode
// - Commit the state transition
// - Perform ENTER actions for the new mode
//
// Notes:
// - This module performs side-effects only (Wi-Fi, power, etc.)
// - UI and display logic react independently via getMode()
// -----------------------------------------------------------------------------

#include <Arduino.h>

#include "system_mode.h"
#include "wifi_manager.h"
#include "Definitions.h"
#include "esp_sleep.h"
#include "gps_manager.h"


// -----------------------------------------------------------------------------
// INTERNAL STATE
// -----------------------------------------------------------------------------
static volatile SystemMode currentMode = MODE_BOOT;

// -----------------------------------------------------------------------------
// PUBLIC API
// -----------------------------------------------------------------------------
SystemMode getMode()
{
  return currentMode;
}

void setMode(SystemMode newMode)
{
  // No-op if already in requested mode
  if (newMode == currentMode) {
    return;
  }

  // ---------------------------------------------------------------------------
  // EXIT actions (based on OLD mode)
  // ---------------------------------------------------------------------------
  switch (currentMode) {

    case MODE_FIELD_CONFIG:
    case MODE_HOME:
      // Leaving any Wi-Fi mode → shut Wi-Fi down cleanly
      wifi_stop();
      break;

    default:
      break;
  }

  // ---------------------------------------------------------------------------
  // STATE TRANSITION
  //
  // IMPORTANT:
  // - This assignment must occur before ENTER actions
  // - ENTER handlers may call getMode()
  // ---------------------------------------------------------------------------
  currentMode = newMode;

  // ---------------------------------------------------------------------------
  // ENTER actions (based on NEW mode)
  //
  // NOTE:
  // - Side-effects only (no UI or rendering)
  // - Display task reacts independently via getMode()
  // ---------------------------------------------------------------------------
  switch (currentMode) {

    case MODE_LOGGING:
      LOG_SYS("MODE", "ENTER LOGGING → WiFi OFF");
      wifi_stop();
      break;

    case MODE_FIELD_CONFIG:
      LOG_SYS("MODE", "ENTER FIELD_CONFIG → WiFi AP");
      wifi_start_ap();
      break;

    case MODE_HOME:
      LOG_SYS("MODE", "ENTER HOME → WiFi STA");
      wifi_start_sta();
      break;

    case MODE_SLEEP:
      LOG_SYS("MODE", "ENTER SLEEP → power off");

      wifi_stop();
      gps_power_off();   // or gps_pause()

      delay(100);        // allow logs to flush

      // Wake when magnet is applied again
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 1);

      esp_deep_sleep_start();
      break;


    default:
      LOG_SYS("MODE", "ENTER UNKNOWN (%d)", currentMode);
      break;
  }
}
