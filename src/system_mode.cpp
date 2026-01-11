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
#include "web/wifi_manager.h"
#include "gps_manager.h"
#include "Display/screen_system.h"

#include "Definitions.h"
#include "esp_sleep.h"
#include "task_display.h"

#include "Storage/storage_file_operations.h"

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
    case MODE_BOOT:          return "BOOT";
    case MODE_WAIT_SATS:     return "WAIT_SATS";
    case MODE_WIFI_SOFT_AP:  return "WIFI_SOFT_AP";
    case MODE_LOGGING:       return "LOGGING";
    case MODE_SLEEP:         return "SLEEP";
    default:                 return "?";
  }
}



void systemModeLoop()
{
  if (getMode() == MODE_WIFI_SOFT_AP) {
    wifi_loop();
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
      Close_files();
      // Leaving primary mission mode
      // (logging task reacts independently)
      break;

    case MODE_WIFI_SOFT_AP:
      // Leaving configuration mode → shut down Wi-Fi
      wifi_stop();
      break;

    case MODE_SLEEP:
      LOG_SYS("MODE", "EXIT SLEEP → power up");
      screen_request_partial(0,0,250,122);
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

    case MODE_WIFI_SOFT_AP:
      // Ensure Serial is alive for config / web diagnostics
      Serial.begin(115200);
      delay(10);
      LOG_SYS("MODE", "ENTER WIFI_SOFT_AP (CONFIG)");
      gps_power_off();

      // Wi-Fi can come up after UI is visible
      wifi_start_ap();
      screen_request_partial(0,0,250,122);
      break;


    case MODE_SLEEP:
      LOG_SYS("MODE", "ENTER SLEEP");
      wifi_stop();
      gps_power_off();
     // screen_request_partial(0,0,250,122);
      break;


    case MODE_BOOT:
    default:
      break;
  }
}
