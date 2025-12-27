// -----------------------------------------------------------------------------
// main.cpp
// System entry point
// -----------------------------------------------------------------------------

#include <Arduino.h>

// -----------------------------------------------------------------------------
// MANAGERS
// -----------------------------------------------------------------------------
#include "boot_manager.h"
#include "wifi_manager.h"
#include "storage_manager.h"
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
#include "E_paper.h"
#include "screen_system.h"
#include "rtc_state.h"

#include <esp_system.h>
#include "Definitions.h"

// -----------------------------------------------------------------------------
// GLOBAL STATE
// -----------------------------------------------------------------------------
bool sleep_mode = false;
extern bool reset_boot;



// Gesture thresholds
static constexpr uint32_t SLEEP_HOLD_MS   = 2000;  // ~2s
static constexpr uint32_t WIFI_HOLD_MS    = 6000;  // ~6s
static constexpr uint32_t BOOT_IGNORE_MS  = 1500;

static constexpr uint32_t LOW_STABLE_MS     = 30;
static constexpr uint32_t RELEASE_STABLE_MS = 40;

// Magnet state
static uint32_t bootTime     = 0;
static uint32_t pressTime   = 0;
static bool     longHandled = false;

// -----------------------------------------------------------------------------
// FORWARD DECLARATIONS
// -----------------------------------------------------------------------------
static void startTasks();
static void heartbeat();
static const char* modeToString(SystemMode mode);
static void magnet_init();
static void magnet_poll();

// -----------------------------------------------------------------------------
// MAGNET INIT
// -----------------------------------------------------------------------------
static void magnet_init()
{
  pinMode(MAGNET_PIN, INPUT);
  bootTime = millis();
}

// -----------------------------------------------------------------------------
// MAGNET POLL (FINAL, CORRECT)
// -----------------------------------------------------------------------------
static void magnet_poll()
{
  const uint32_t now = millis();

  // Ignore just after boot / wake
  if (now - bootTime < BOOT_IGNORE_MS) return;

  // LOW = magnet present
  const bool rawLow = (digitalRead(MAGNET_PIN) == LOW);

  // LOW debounce
  static uint32_t lowSince = 0;
  if (rawLow) {
    if (lowSince == 0) lowSince = now;
  } else {
    lowSince = 0;
  }
  const bool active = (lowSince && (now - lowSince >= LOW_STABLE_MS));

  // Release debounce
  static uint32_t releaseSince = 0;
  if (!active) {
    if (releaseSince == 0) releaseSince = now;
  } else {
    releaseSince = 0;
  }
  const bool releasedStable =
      (releaseSince && (now - releaseSince >= RELEASE_STABLE_MS));

  static bool prevActive = false;

  // --------------------------------------------------
  // PRESS EDGE
  // --------------------------------------------------
  if (active && !prevActive) {
    pressTime   = now;
    longHandled = false;
  }

  // --------------------------------------------------
  // LONG HOLD (≥6s) → TOGGLE WIFI
  // --------------------------------------------------
  if (active &&
      !longHandled &&
      (now - pressTime >= WIFI_HOLD_MS)) {

    longHandled = true;

    if (getMode() == MODE_FIELD_CONFIG) {
      setMode(MODE_LOGGING);        // Exit Wi-Fi
    } else {
      setMode(MODE_FIELD_CONFIG);   // Enter Wi-Fi
    }
  }

  // --------------------------------------------------
  // RELEASE (2–6s) → SLEEP / WAKE
  // --------------------------------------------------
 if (!active && prevActive && !longHandled) {

  const uint32_t held = now - pressTime;

  if (held >= SLEEP_HOLD_MS && held < WIFI_HOLD_MS) {
    if (getMode() == MODE_SLEEP) {
      setMode(MODE_LOGGING);
    } else {
      setMode(MODE_SLEEP);
    }
  }
 }

  prevActive = active;
}

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------
void setup()
{
  initBoot();
  initStorage();
  initConfig();
  initGPS();

  magnet_init();
  setMode(MODE_LOGGING);

  startTasks();
}

// -----------------------------------------------------------------------------
// TASK STARTUP
// -----------------------------------------------------------------------------
static void startTasks()
{
  BaseType_t ok;

  ok = xTaskCreatePinnedToCore(taskOne, "TaskGPS",
                              10000, nullptr, 1, &t1, 1);
  if (ok != pdPASS) Serial.println("[TASK] GPS create failed");

  ok = xTaskCreatePinnedToCore(taskTwo, "TaskDisplay",
                              10000, nullptr, 1, &t2, 0);
  if (ok != pdPASS) Serial.println("[TASK] Display create failed");

  Serial.print("[TASK] started");
  if (t1) { Serial.print(" t1_hw="); Serial.print(uxTaskGetStackHighWaterMark(t1)); }
  if (t2) { Serial.print(" t2_hw="); Serial.print(uxTaskGetStackHighWaterMark(t2)); }
  Serial.println();
}

// -----------------------------------------------------------------------------
// LOOP
// -----------------------------------------------------------------------------
void loop()
{
  magnet_poll();
  watchdogLoop();

  const SystemMode mode = getMode();
  if (mode == MODE_HOME || mode == MODE_FIELD_CONFIG) {
    wifi_loop();
  }

  heartbeat();
  delay(10);
}

// -----------------------------------------------------------------------------
// HEARTBEAT
// -----------------------------------------------------------------------------
static void heartbeat()
{
  static uint32_t last = 0;

  if (millis() - last > 3000) {
    last = millis();

    Serial.print("[LOOP] mode=");
    Serial.print(modeToString(getMode()));
    Serial.print(" heap=");
    Serial.print(ESP.getFreeHeap());
    Serial.print(" min=");
    Serial.print(esp_get_minimum_free_heap_size());

    if (t1) { Serial.print(" t1_hw="); Serial.print(uxTaskGetStackHighWaterMark(t1)); }
    if (t2) { Serial.print(" t2_hw="); Serial.print(uxTaskGetStackHighWaterMark(t2)); }

    Serial.println();
  }
}

// -----------------------------------------------------------------------------
// MODE → STRING
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
