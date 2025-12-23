#include <Arduino.h>

// Managers
#include "boot_manager.h"
#include "wifi_manager.h"
#include "storage_manager.h"
#include "config_manager.h"
#include "eeprom_manager.h"
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

// -----------------------------------------------------------------------------
// EXTERNAL / RTC STATE
// -----------------------------------------------------------------------------
extern RTC_DATA_ATTR int  RTC_Sail_Logo;
extern RTC_DATA_ATTR char RTC_Sleep_txt[32];

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
  initBoot();        // hardware + boot screen
  initStorage();     // SD + LittleFS
  initConfig();      // JSON config

  // Decide initial system mode ONLY
  // (does not start Wi-Fi directly)
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
