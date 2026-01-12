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

#include "Storage/storage_manager.h"
#include "Storage/storage_file_operations.h"

// -----------------------------------------------------------------------------
// INTERNAL STATE
// -----------------------------------------------------------------------------
static volatile SystemMode currentMode = MODE_BOOT;

// -----------------------------------------------------------------------------
// PUBLIC API
// -----------------------------------------------------------------------------
SystemMode getMode(){return currentMode;}

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
  if (getMode() == MODE_WIFI_SOFT_AP) {wifi_loop(); }
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

  LOG_SYS("MODE", "EXIT %s → ENTER %s", modeToString(currentMode),modeToString(newMode));

  // ---------------------------------------------------------------------------
  // EXIT actions (based on OLD mode)
  // ---------------------------------------------------------------------------
  switch (currentMode) {

    case MODE_LOGGING:
      LOG_SYS("MODE", "EXIT LOGGING");

      storage_shutting_down = true;   // <-- ADD THIS
      delay(20);                      // allow in-flight writes to finish

      Close_files();
      storage_off();
      break;


    case MODE_WIFI_SOFT_AP:
      storage_shutting_down = true;        
      wifi_stop();
      storage_off();   
      break;

    case MODE_SLEEP:
      LOG_SYS("MODE", "EXIT SLEEP → power up");
      screen_request_partial(0,0,250,122);
       break;

    case MODE_BOOT:
    default:
      break;
  }

  currentMode = newMode;

  // ---------------------------------------------------------------------------
  // ENTER actions (based on NEW mode)
  //
  // ---------------------------------------------------------------------------
  switch (currentMode) {

    case MODE_LOGGING:
      LOG_SYS("MODE", "ENTER LOGGING → Wi-Fi OFF");
      storage_shutting_down = false;
      wifi_stop();
      gps_power_on();

      if (!storage_on()) {
        LOG_ERROR("SD", "storage_on failed → abort logging");
        // Optional: force fallback mode here
        // setMode(MODE_SLEEP);
        break;
      }

      Open_files();   // start session files only AFTER SD is mounted
      break;

    case MODE_WIFI_SOFT_AP:
      storage_shutting_down = false;

      Serial.begin(115200);
      delay(10);
      LOG_SYS("MODE", "ENTER WIFI_SOFT_AP (CONFIG)");

      gps_power_off();

      if (!storage_on()) {
        LOG_ERROR("SD", "storage_on failed in CONFIG");
        // Optional: still allow config via LittleFS-only
      }

      wifi_start_ap();
      screen_request_partial(0,0,250,122);
      break;

   case MODE_SLEEP:
      LOG_SYS("MODE", "ENTER SLEEP");

      wifi_stop();
      gps_power_off();

      storage_off();   
      break;

    case MODE_BOOT:
    default:
      break;
  }
}
