// ============================================================================
// main.cpp
//
// System entry point.
// Consumes user intent via magnet_input.
// All behaviour flows through setMode().
// ============================================================================

#include <Arduino.h>

// --- Managers ----------------------------------------------------------------
#include "boot_manager.h"
#include "Storage/storage_manager.h"
#include "config_manager.h"
#include "gps_manager.h"
#include "watchdog_manager.h"

// --- Tasks -------------------------------------------------------------------
#include "task_gps.h"
#include "task_display.h"

// --- System / State ----------------------------------------------------------
#include "system_mode.h"
#include "rtc_state.h"
#include "Definitions.h"
// --- Input -------------------------------------------------------------------
#include "magnet_input.h"

// --- Config support (serviced via systemModeLoop) ----------------------------
#include "web/wifi_manager.h"

// --- Forward declarations ----------------------------------------------------
static void startTasks();

// ============================================================================
// Setup
// ============================================================================
void setup()
{
  const BootResult br = initBoot();
  if (br != BOOT_OK) {

    const char* reason = bootFailReason();
   

    setMode(MODE_SLEEP);
    return;
  }

  initStorage();
  initConfig();
  initGPS();
  magnet_init();

  startTasks();
  setMode(MODE_WAIT_SATS);
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

  ok = xTaskCreatePinnedToCore(taskOne, "TaskGPS", 6096, nullptr, 1, &t1, 1);
  if (ok != pdPASS) LOG_TASK("Create", "GPS task failed");

  ok = xTaskCreatePinnedToCore(taskTwo, "TaskDisplay", 6096, nullptr, 1, &t2, 0);
  if (ok != pdPASS) LOG_TASK("Create", "Display task failed");

  LOG_TASK("Start", "tasks started");


  if (t1) LOG_TASK("stack", "t1_hw=%u", uxTaskGetStackHighWaterMark(t1));
  if (t2) LOG_TASK("stack", "t2_hw=%u", uxTaskGetStackHighWaterMark(t2));

 
}
