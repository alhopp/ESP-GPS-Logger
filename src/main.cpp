// -----------------------------------------------------------------------------
// main.cpp
//
// System entry point:
// - Runs ordered startup (boot → storage → config → Wi-Fi mode selection)
// - Starts core FreeRTOS tasks (GPS, display)
// - Services Wi-Fi and watchdog in the main loop
// - Provides a lightweight heartbeat for bring-up diagnostics
//
// All hardware and subsystem initialization is delegated to managers.
// -----------------------------------------------------------------------------


#include <Arduino.h>

// Managers
#include "boot_manager.h"
#include "wifi_manager.h"
#include "storage_manager.h"
#include "config_manager.h"
#include "watchdog_manager.h"

// Tasks
#include "task_gps.h"
#include "task_display.h"

// System
#include "system_mode.h"

// UI / misc
#include "E_paper.h"
#include "screen_system.h"

// ESP32 heap stats
#include <esp_system.h>

#include "rtc_state.h"

bool sleep_mode = false;
extern bool reset_boot;

// -----------------------------------------------------------------------------
// FORWARD DECLARATIONS
// -----------------------------------------------------------------------------
static void startTasks();
static void heartbeat();
static const char* modeToString(SystemMode mode);

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------
void setup()
{

  // ---------------------------------------------------------------------------
  // Early boot:
  // - Initializes Serial, battery ADC, SPI, system time
  // - Brings up the e-paper display and shows the boot screen
  // - Enforces hard shutdown on low battery or reset boot
  //
  // Must run before storage, config, Wi-Fi, or tasks.
  // ---------------------------------------------------------------------------
  initBoot();        // hardware + boot screen
  
  // ---------------------------------------------------------------------------
  // Storage:
  // - Mounts SD card if present (optional)
  // - Mounts LittleFS (mandatory)
  // - Performs basic I/O sanity check
  // Must run before config loading, logging, or data access.
  // ---------------------------------------------------------------------------
  initStorage();     // SD + LittleFS

  // ---------------------------------------------------------------------------
  // Configuration:
  // - Loads configuration from LittleFS (config.txt)
  // - Creates and saves defaults if missing or invalid
  // - Applies derived runtime values (RTC, calibration, UI settings)
  //
  // Must run after storage init and before Wi-Fi, logging, or tasks.
  // ---------------------------------------------------------------------------
  initConfig();      // JSON config
 
 

  // ---------------------------------------------------------------------------
  // Wi-Fi mode selection:
  // - Loads saved Wi-Fi credentials (if present)
  // - Selects initial system mode (HOME or FIELD_CONFIG)
  //
  // Does NOT start Wi-Fi or networking yet.
  // Actual Wi-Fi setup is handled later by the mode manager.
  // ---------------------------------------------------------------------------
  initWifi();


  // Start FreeRTOS tasks
  startTasks();
}

// -----------------------------------------------------------------------------
// TASK STARTUP
// -----------------------------------------------------------------------------
static void startTasks()
{
  // GPS task (core 1)
  xTaskCreatePinnedToCore(
    taskOne,
    "TaskGPS",
    10000,
    nullptr,
    1,
    &t1,
    1
  );

  // Display task (core 0)
  xTaskCreatePinnedToCore(
    taskTwo,
    "TaskDisplay",
    10000,
    nullptr,
    1,
    &t2,
    0
  );
}

// -----------------------------------------------------------------------------
// LOOP
// -----------------------------------------------------------------------------
void loop()
{
  watchdogLoop();

  const SystemMode mode = getMode();

  // Wi-Fi is serviced ONLY in Wi-Fi modes
  if (mode == MODE_HOME || mode == MODE_FIELD_CONFIG) {
    wifi_loop();
  }

  // Lightweight heartbeat for bring-up / sanity
  heartbeat();

  // Yield to FreeRTOS (loop is not time-critical)
  delay(10);
}

// -----------------------------------------------------------------------------
// HEARTBEAT (BRING-UP / DIAGNOSTIC)
// -----------------------------------------------------------------------------
static void heartbeat()
{
  static uint32_t last = 0;

  if (millis() - last > 3000) {
    last = millis();

    const SystemMode mode = getMode();

    Serial.print("[LOOP   ] mode=");
    Serial.print(modeToString(mode));

    Serial.print(" heap=");
    Serial.print(ESP.getFreeHeap());

    Serial.print(" min=");
    Serial.println(esp_get_minimum_free_heap_size());
  }
}

// -----------------------------------------------------------------------------
// MODE → STRING (UI / LOGGING ONLY)
// -----------------------------------------------------------------------------
static const char* modeToString(SystemMode mode)
{
  switch (mode) {
    case MODE_BOOT:         return "BOOT";
    case MODE_LOGGING:      return "LOGGING";
    case MODE_FIELD_CONFIG: return "FIELD_CFG";
    case MODE_HOME:         return "HOME";
    case MODE_SLEEP:        return "SLEEP";
    default:                return "?";
  }
}
