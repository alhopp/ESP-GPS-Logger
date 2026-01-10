// -----------------------------------------------------------------------------
// main.cpp
//
// System entry point.
//
// Responsibilities:
// - System startup and task creation
// - User intent interpretation (magnet)
// - Periodic servicing (watchdog, Wi-Fi, heartbeat)
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

// -----------------------------------------------------------------------------
// CONFIGURATION / THRESHOLDS
// -----------------------------------------------------------------------------

// Gesture thresholds ms
static constexpr uint32_t SLEEP_HOLD_MS   = 1000;  
static constexpr uint32_t WIFI_HOLD_MS    = 4000;
static constexpr uint32_t BOOT_IGNORE_MS  = 1500;

static constexpr uint32_t LOW_STABLE_MS     = 30;
static constexpr uint32_t RELEASE_STABLE_MS = 40;

// -----------------------------------------------------------------------------
// RUNTIME STATE
// -----------------------------------------------------------------------------

// Magnet timing state
static uint32_t bootTime    = 0;
static uint32_t pressTime   = 0;
static bool     longHandled = false;

// -----------------------------------------------------------------------------
// FORWARD DECLARATIONS
// -----------------------------------------------------------------------------
static void startTasks();
static void heartbeat();
static void magnet_init();
static void magnet_poll();

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

  heartbeat();
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

// -----------------------------------------------------------------------------
// MAGNET INIT
// -----------------------------------------------------------------------------
static void magnet_init()
{
  pinMode(MAGNET_PIN, INPUT);
  bootTime = millis();
}

// -----------------------------------------------------------------------------
// MAGNET POLL (authoritative user intent interpreter)
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

  static bool prevActive = false;

  // --------------------------------------------------
  // PRESS EDGE
  // --------------------------------------------------
  if (active && !prevActive) {
    pressTime   = now;
    longHandled = false;
  }

  // --------------------------------------------------
  // LONG HOLD (≥ WIFI_HOLD_MS) → CONFIG MODE
  // --------------------------------------------------
  if (active &&
      !longHandled &&
      (now - pressTime >= WIFI_HOLD_MS)) {

    longHandled = true;

    if (getMode() == MODE_WIFI_SOFT_AP) {
      setMode(MODE_LOGGING);
       screen_request_partial(0,0,250,122);
    } else {
      setMode(MODE_WIFI_SOFT_AP);
    }
  }

  // --------------------------------------------------
  // RELEASE (≥ SLEEP_HOLD_MS, < WIFI_HOLD_MS) → SLEEP
  // --------------------------------------------------
  if (!active && prevActive && !longHandled) {

    const uint32_t held = now - pressTime;

    if (held >= SLEEP_HOLD_MS && held < WIFI_HOLD_MS) {
      if (getMode() == MODE_SLEEP) {
        setMode(MODE_LOGGING);
               screen_request_partial(0,0,250,122);
      } else {
        setMode(MODE_SLEEP);
            screen_request_partial(0,0,250,122);
      }
    }
  }

  prevActive = active;
}



// ============================================================================
// HEARTBEAT / DIAGNOSTICS
// ============================================================================
static void heartbeat()
{
  static uint32_t last = 0;

  if (millis() - last > 3000) {
    last = millis();

  if (wifi_sta_connected()) {
   // Serial.print("UP ssid=");
   // Serial.print(wifi_sta_ssid());
   // Serial.print(" ip=");
   // Serial.print(wifi_sta_ip());
  }
  else if (WiFi.status() == WL_CONNECTED) {
    //Serial.print("UP (pending)");
  }
  else {
   // Serial.print("DOWN");
  }

  //Serial.println();

  }
}
