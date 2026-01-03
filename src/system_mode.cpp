// -----------------------------------------------------------------------------
// system_mode.cpp
//
// Central system mode state machine.
//
// Responsibilities:
// - Own the authoritative SystemMode
// - Execute EXIT actions for the old mode
// - Commit the state transition
// - Execute ENTER actions for the new mode
//
// Design rules:
// - Side-effects ONLY (Wi-Fi, GPS, power, sleep)
// - No UI, no drawing, no rendering
// - Display logic reacts independently via getMode()
// -----------------------------------------------------------------------------

#include <Arduino.h>

#include "system_mode.h"
#include "wifi_manager.h"
#include "gps_manager.h"
#include "screen_system.h"

#include "Definitions.h"
#include "esp_sleep.h"

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

// -----------------------------------------------------------------------------
// MODE → STRING (debug / logging only)
// -----------------------------------------------------------------------------
const char* modeToString(SystemMode mode)
{
  switch (mode) {
    case MODE_BOOT:         return "BOOT";
    case MODE_LOGGING:      return "LOGGING";
    case MODE_FIELD_CONFIG: return "FIELD_CFG";
    case MODE_SLEEP:        return "SLEEP";
    default:                return "?";
  }
}

// -----------------------------------------------------------------------------
// STATE TRANSITION
// -----------------------------------------------------------------------------
void setMode(SystemMode newMode)
{
  // ---------------------------------------------------------------------------
  // No-op if already in requested mode
  // ---------------------------------------------------------------------------
  if (newMode == currentMode) {
    return;
  }

  LOG_SYS("MODE", "EXIT %s → ENTER %s",
          modeToString(currentMode),
          modeToString(newMode));

  // ---------------------------------------------------------------------------
  // EXIT actions (based on OLD mode)
  // ---------------------------------------------------------------------------
  switch (currentMode) {

    case MODE_LOGGING:
      // Leaving primary mission mode
      // (logging task reacts independently)
      break;

    case MODE_FIELD_CONFIG:
      // Leaving configuration mode → shut down Wi-Fi
      wifi_stop();
      break;

    case MODE_SLEEP:
      // Should not normally exit sleep
      break;

    case MODE_BOOT:
    default:
      break;
  }

  // ---------------------------------------------------------------------------
  // STATE COMMIT
  //
  // IMPORTANT:
  // - Assignment MUST occur before ENTER actions
  // - ENTER handlers may legally call getMode()
  // ---------------------------------------------------------------------------
  currentMode = newMode;

  // ---------------------------------------------------------------------------
  // ENTER actions (based on NEW mode)
  //
  // Rules:
  // - Side-effects only
  // - No UI, no drawing, no rendering
  // ---------------------------------------------------------------------------
  switch (currentMode) {

    case MODE_LOGGING:
      LOG_SYS("MODE", "ENTER LOGGING → Wi-Fi OFF");

      wifi_stop();
      gps_power_on();  
      break;

    case MODE_FIELD_CONFIG:
      LOG_SYS("MODE", "ENTER FIELD CONFIG → Wi-Fi AP, GPS OFF");

      gps_power_off();
      wifi_start_ap();
      break;

    case MODE_SLEEP:
      LOG_SYS("MODE", "ENTER SLEEP → power down");

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

    case MODE_BOOT:
    default:
      break;
  }

  // ---------------------------------------------------------------------------
  // Notify display task that mode has changed
  // (no drawing here — display task owns rendering)
  // ---------------------------------------------------------------------------
  // screen_request_redraw();
}
