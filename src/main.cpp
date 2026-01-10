// -----------------------------------------------------------------------------
// main.cpp
//
// System entry point.
//
// Responsibilities:
// - System startup and task creation
// - User intent interpretation (magnet)
//
// Design rules:
// - main.cpp owns *intent*, not side-effects
// - All mode changes go through setMode()
// -----------------------------------------------------------------------------

#include <Arduino.h>

// -----------------------------------------------------------------------------
// MANAGERS
// -----------------------------------------------------------------------------
#include "boot_manager.h"
#include "web/wifi_manager.h"
#include "web/web_server.h"

#include "Storage/storage_manager.h"
#include "config_manager.h"
#include "gps_manager.h"
#include "watchdog_manager.h"

// -----------------------------------------------------------------------------
// TASKS
// -----------------------------------------------------------------------------
#include "task_gps.h"
#include "task_display.h"

// -----------------------------------------------------------------------------
// SYSTEM / UI
// -----------------------------------------------------------------------------
#include "system_mode.h"
#include "Definitions.h"
#include "rtc_state.h" 

#include "magnet_input.h" 

// -----------------------------------------------------------------------------
// FORWARD DECLARATIONS
// -----------------------------------------------------------------------------
static void startTasks();

// ============================================================================
// SETUP
// ============================================================================
void setup()
{
  const BootResult br = initBoot();

  // Fatal boot outcomes are decided here 
  if (br != BOOT_OK) {

    // Record reason for display/sleep screen
    RTC_OFF_screen = 1;

    const char* reason = bootFailReason();
    if (reason && reason[0]) {
      strncpy(RTC_Sleep_txt, reason, sizeof(RTC_Sleep_txt) - 1);
      RTC_Sleep_txt[sizeof(RTC_Sleep_txt) - 1] = '\0';
    } else {
      strncpy(RTC_Sleep_txt, "Boot failed", sizeof(RTC_Sleep_txt) - 1);
      RTC_Sleep_txt[sizeof(RTC_Sleep_txt) - 1] = '\0';
    }

    // Now commit the system decision
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
// LOOP
// ============================================================================
void loop()
{
  magnet_poll();
  watchdogLoop();

  // Service Wi-Fi only in config mode
  if (getMode() == MODE_WIFI_SOFT_AP) {
    wifi_loop();
  }

  delay(10);
}

// ============================================================================
// TASK STARTUP
// ============================================================================
static void startTasks()
{
  BaseType_t ok;

  ok = xTaskCreatePinnedToCore(taskOne, "TaskGPS",
                              10000, nullptr, 1, &t1, 1);
  if (ok != pdPASS) {
    LOG_TASK("Create", "GPS task failed");
  }

  ok = xTaskCreatePinnedToCore(taskTwo, "TaskDisplay",
                              10000, nullptr, 1, &t2, 0);
 
  if (ok != pdPASS) {
    Serial.println("[TASK   ] Display create failed");
  }

  LOG_TASK("Start", "tasks started");

  if (t1) {Serial.printf("[TASK   ] t1_hw=%u\r\n",
                  uxTaskGetStackHighWaterMark(t1));
  }

  if (t2) {Serial.printf("[TASK   ] t2_hw=%u\r\n",
                  uxTaskGetStackHighWaterMark(t2));
  }

  Serial.println();

}
