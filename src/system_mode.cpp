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
#include "screen_system.h"

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

    case MODE_LOGGING:
    case MODE_SLEEP:
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
      gps_power_off(); 
      wifi_start_ap();
      FieldAP_screen();



      break;

    case MODE_SLEEP:
      LOG_SYS("MODE", "ENTER SLEEP → power off");

      wifi_stop();
      gps_power_off();

      delay(100);

      // Ensure magnet is RELEASED before sleeping
      while (digitalRead(MAGNET_PIN) == LOW) {
        delay(10);
      }

      // Wake when magnet is applied (LOW)
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 0);

      esp_deep_sleep_start();
      break;


    default:
      LOG_SYS("MODE", "ENTER UNKNOWN (%d)", currentMode);
      break;
  }
}
