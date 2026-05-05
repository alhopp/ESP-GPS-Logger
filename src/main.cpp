// ============================================================================
// main.cpp
// - System entry point
// - Boot → init subsystems → start RTOS tasks → enter idle mode
// - loop() acts only as a lightweight supervisor
// ============================================================================

#include <Arduino.h>

// --- Core managers -----------------------------------------------------------
#include "storage/storage_manager.h"
#include "managers/boot_manager.h"
#include "managers/config_manager.h"
#include "managers/watchdog_manager.h"
#include "GPS/gps_manager.h"

// --- Tasks ------------------------------------------------------------------
#include "tasks/task_runtime.h"

// --- System / input ----------------------------------------------------------
#include "core/sleep_control.h"
#include "core/system_mode.h"
#include "core/magnet_input.h"
#include "core/Definitions.h"
#include "core/Globals.h"

// --- Local config ------------------------------------------------------------
namespace {
constexpr uint32_t LOOP_DELAY_MS = 10;
}

// ============================================================================
// Setup (runs once at boot)
// ============================================================================
void setup() {
  LOG_SYS("Setup", "start");

  setMode(MODE_BOOT);

  // Detect wake source (used globally)
  woke_from_sleep = sleep_woke_from_magnet();

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
  if (!startRuntimeTasks()) {
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
