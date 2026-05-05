// ============================================================================
// main.cpp
// - System entry point
// - Boot → init subsystems → start RTOS tasks → enter idle mode
// - loop() acts only as a lightweight supervisor
// ============================================================================

#include <Arduino.h>

// --- Core managers -----------------------------------------------------------
#include "Storage/storage_manager.h"
#include "System/boot_manager.h"
#include "Config/config_manager.h"
#include "System/watchdog_manager.h"
#include "GPS/gps_manager.h"

// --- Tasks ------------------------------------------------------------------
#include "Runtime/task_runtime.h"

// --- System / input ----------------------------------------------------------
#include "Core/sleep_control.h"
#include "Core/system_mode.h"
#include "Core/magnet_input.h"
#include "Core/Definitions.h"
#include "Core/Globals.h"

// --- Local config ------------------------------------------------------------
namespace {
constexpr uint32_t LOOP_DELAY_MS = 10;

void initSubsystems()
{
  initStorage();
  initConfig();
  initGPS();
  initMagnet();
}

bool startTasksOrEnterError()
{
  if (startRuntimeTasks()) {
    return true;
  }

  setMode(MODE_ERROR);
  return false;
}
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
  initSubsystems();

  // Start runtime tasks
  if (!startTasksOrEnterError()) {
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
