// ============================================================================
// main.cpp
//
// System entry point.
// - Performs early boot validation
// - Initialises core managers
// - Starts RTOS tasks
// - All runtime behaviour flows through setMode()
// ============================================================================

#include <Arduino.h>

// --- Core managers ------------------------------------------------------------

#include "Storage/storage_manager.h"
#include "Display/screen_system.h"

#include "MANAGERS/boot_manager.h"
#include "MANAGERS/config_manager.h"
#include "MANAGERS/watchdog_manager.h"


#include "GPS/gps_manager.h"
// --- Tasks -------------------------------------------------------------------
#include "task_gps.h"
#include "task_display.h"

// --- System / input -----------------------------------------------------------
#include "system_mode.h"
#include "magnet_input.h"
#include "Core/Definitions.h"

#include "Core/Globals.h"

// --- Forward declarations -----------------------------------------------------
static void startTasks();



// ============================================================================
// Setup
// ============================================================================
void setup()
{
  woke_from_sleep =
    (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1);

  const BootResult br = initBoot();
  if (br != BOOT_OK) {
    setMode(MODE_SLEEP);
    return;
  }

  initStorage();
  initConfig();
  initGPS();
  magnet_init();
  startTasks();

  // IMPORTANT: leave BOOT mode
  setMode(MODE_IDLE);
}


// ============================================================================
// Loop
// ============================================================================
void loop()
{
  magnet_poll();

  watchdogLoop();
  systemModeLoop();
  
  delay(10);
}

// ============================================================================
// Task startup
// ============================================================================
static void startTasks()
{
  BaseType_t ok;

  ok = xTaskCreatePinnedToCore(taskOne, "TaskGPS",     6096, nullptr, 1, &t1, 1);
  if (ok != pdPASS) LOG_TASK("Create", "GPS task failed");

  ok = xTaskCreatePinnedToCore(taskTwo, "TaskDisplay", 6096, nullptr, 1, &t2, 0);
  if (ok != pdPASS) LOG_TASK("Create", "Display task failed");

  LOG_TASK("Start", "tasks started");

  if (t1) LOG_TASK("stack", "t1_hw=%u", uxTaskGetStackHighWaterMark(t1));
  if (t2) LOG_TASK("stack", "t2_hw=%u", uxTaskGetStackHighWaterMark(t2));
}
