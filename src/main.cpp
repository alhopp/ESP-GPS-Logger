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
    RTC_OFF_screen = 1;

    const char* reason = bootFailReason();
    strncpy(RTC_Sleep_txt,
            (reason && reason[0]) ? reason : "Boot failed",
            sizeof(RTC_Sleep_txt) - 1);
    RTC_Sleep_txt[sizeof(RTC_Sleep_txt) - 1] = '\0';

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

  ok = xTaskCreatePinnedToCore(taskOne, "TaskGPS", 10000, nullptr, 1, &t1, 1);
  if (ok != pdPASS) LOG_TASK("Create", "GPS task failed");

  ok = xTaskCreatePinnedToCore(taskTwo, "TaskDisplay", 10000, nullptr, 1, &t2, 0);
  if (ok != pdPASS) LOG_TASK("Create", "Display task failed");

  LOG_TASK("Start", "tasks started");

  if (t1) Serial.printf("[TASK   ] t1_hw=%u\r\n", uxTaskGetStackHighWaterMark(t1));
  if (t2) Serial.printf("[TASK   ] t2_hw=%u\r\n", uxTaskGetStackHighWaterMark(t2));

  Serial.println();
}
