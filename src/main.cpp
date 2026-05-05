// ============================================================================
// main.cpp
// - System entry point
// - Boot → init subsystems → start RTOS tasks → enter idle mode
// - loop() acts only as a lightweight supervisor
// ============================================================================

#include <Arduino.h>
#include <esp_sleep.h>

// --- Core managers -----------------------------------------------------------
#include "storage/storage_manager.h"
#include "managers/boot_manager.h"
#include "managers/config_manager.h"
#include "managers/watchdog_manager.h"
#include "GPS/gps_manager.h"

// --- Tasks ------------------------------------------------------------------
#include "tasks/task_gps.h"
#include "tasks/task_display.h"

// --- System / input ----------------------------------------------------------
#include "core/system_mode.h"
#include "core/magnet_input.h"
#include "core/Definitions.h"
#include "core/Globals.h"

// --- Local config ------------------------------------------------------------
namespace {
constexpr uint32_t GPS_TASK_STACK     = 4096;
constexpr UBaseType_t GPS_TASK_PRIO   = 2;

constexpr uint32_t DISPLAY_TASK_STACK = 6096;
constexpr UBaseType_t DISPLAY_TASK_PRIO = 1;

constexpr uint32_t LOOP_DELAY_MS = 10;

// Create RTOS tasks (GPS + Display)
// Returns false if any task fails
static bool startTasks() {
  t1 = t2 = nullptr;

  // GPS task (core 1)
  if (xTaskCreatePinnedToCore(taskOne, "TaskGPS", GPS_TASK_STACK, nullptr, GPS_TASK_PRIO, &t1, 1) != pdPASS) {
    LOG_SYS("Task", "GPS task create failed");
    return false;
  }

  // Display task (core 0)
  if (xTaskCreatePinnedToCore(taskTwo, "TaskDisplay", DISPLAY_TASK_STACK, nullptr, DISPLAY_TASK_PRIO, &t2, 0) != pdPASS) {
    LOG_SYS("Task", "Display task create failed");
    if (t1) { vTaskDelete(t1); t1 = nullptr; }
    return false;
  }

  LOG_SYS("Task", "tasks started");
  if (t1) LOG_SYS("Task", "GPS hw=%u", uxTaskGetStackHighWaterMark(t1));
  if (t2) LOG_SYS("Task", "Display hw=%u", uxTaskGetStackHighWaterMark(t2));

  return true;
}
}

// ============================================================================
// Setup (runs once at boot)
// ============================================================================
void setup() {
  LOG_SYS("Setup", "start");

  setMode(MODE_BOOT);

  // Detect wake source (used globally)
  woke_from_sleep = (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1);

  // Early boot validation
  if (initBoot() != BOOT_OK) {
    setMode(MODE_SLEEP);
    return;
  }

  // Init order matters: storage → config → GPS → input
  initStorage();
  initConfig();
  initGPS();
  initMagnet();

  // Start runtime tasks
  if (!startTasks()) {
    setMode(MODE_ERROR);
    return;
  }

  setMode(MODE_IDLE);
  LOG_SYS("Setup", "done");
}

// ============================================================================
// Loop (runs continuously)
// ============================================================================
void loop() {
  magnet_poll();
  watchdogLoop();
  systemModeLoop();
  delay(LOOP_DELAY_MS);
}