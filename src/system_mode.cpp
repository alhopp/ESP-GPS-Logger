// -----------------------------------------------------------------------------
// system_mode.cpp
//
// Central system mode state machine.
//
// This module owns the authoritative SystemMode and is responsible for
// performing *side-effect transitions* between modes:
//
//   - Wi-Fi on/off
//   - GPS power control
//   - Storage mount / unmount
//   - Sleep entry / exit preparation
//
// Design rules:
// - NO rendering, NO UI logic, NO drawing
// - NO business logic
// - Side-effects ONLY
// - Display reacts independently via getMode()
// - Storage lifecycle is explicitly managed at mode boundaries
// -----------------------------------------------------------------------------


#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

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
// Single source of truth for system mode.
// Volatile because it is read by multiple tasks.
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
    case MODE_IDLE:          return "IDLE";
    case MODE_WAIT_SATS:     return "WAIT_SATS";
    case MODE_WIFI_SOFT_AP:  return "WIFI_SOFT_AP";
    case MODE_LOGGING:       return "LOGGING";
    case MODE_SLEEP:         return "SLEEP";
    default:                 return "?";
  }
}


// -----------------------------------------------------------------------------
// MODE-SPECIFIC LOOP HOOK
// -----------------------------------------------------------------------------
void systemModeLoop()
{
  // Only CONFIG mode has a live event loop
  if (getMode() == MODE_WIFI_SOFT_AP) {
    wifi_loop();
  }
}


// -----------------------------------------------------------------------------
// STATE TRANSITION
// -----------------------------------------------------------------------------
void setMode(SystemMode newMode)
{

  LOG_SYS("MODE", "REQUEST %s", modeToString(newMode));
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
  //
  // IMPORTANT:
  // - These actions complete BEFORE the state commit
  // - They are allowed to assume the OLD mode is still active
  // ---------------------------------------------------------------------------
  switch (currentMode) {

    case MODE_LOGGING:
      // Signal all storage writers to stop immediately
      storage_shutting_down = true;

      // Allow in-flight SD writes to drain safely
      vTaskDelay(pdMS_TO_TICKS(20));

      // Close files first, then unmount storage
      Close_files();
      storage_off();
      break;

    case MODE_WIFI_SOFT_AP:
      // Configuration mode exit:
      // stop SD activity, then shut down Wi-Fi
      storage_shutting_down = true;
      wifi_stop();
      storage_off();
      break;

    case MODE_SLEEP:
      // Waking from sleep — hardware re-enable happens in ENTER
      LOG_SYS("MODE", "EXIT SLEEP → power up");
      break;

    case MODE_BOOT:
    default:
      break;
  }

  // ---------------------------------------------------------------------------
  // STATE COMMIT
  //
  // MUST happen before ENTER actions.
  // ENTER handlers may legally call getMode().
  // ---------------------------------------------------------------------------
  currentMode = newMode;
  screen_request_partial(0, 0, 250, 122);
  
  // ---------------------------------------------------------------------------
  // ENTER actions (based on NEW mode)
  //
  // Rules:
  // - Side-effects ONLY
  // - No rendering
  // - No long-running logic
  // ---------------------------------------------------------------------------
  switch (currentMode) {

    case MODE_LOGGING:
      LOG_SYS("MODE", "ENTER LOGGING → Wi-Fi OFF");

      storage_shutting_down = false;

      wifi_stop();
      gps_power_on();

      // Storage must be mounted BEFORE any files are opened
      if (!storage_on()) {
        LOG_ERROR("SD", "storage_on failed → abort logging");
        break;
      }

      Open_files();   // session files start here
      break;

    case MODE_WIFI_SOFT_AP:
      storage_shutting_down = false;

      Serial.begin(115200);
      vTaskDelay(pdMS_TO_TICKS(10));

      LOG_SYS("MODE", "ENTER WIFI_SOFT_AP (CONFIG)");

      gps_power_off();

      // SD is optional but preferred for file manager access
      if (!storage_on()) {
        LOG_ERROR("SD", "storage_on failed in CONFIG");
        // System continues using LittleFS only
      }

      wifi_start_ap();
      break;

    case MODE_SLEEP:
      LOG_SYS("MODE", "ENTER SLEEP");

      wifi_stop();
      gps_power_off();

      // Final defensive unmount before deep sleep
      storage_off();
      break;

    case MODE_BOOT:
    default:
      break;
  }
}
