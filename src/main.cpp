// -----------------------------------------------------------------------------
// main.cpp
//
// System entry point.
//
// Responsibilities:
// - Perform ordered system startup:
//     boot → storage → configuration → GPS
// - Select the initial operating mode (default: GPS logging)
// - Launch core FreeRTOS tasks (GPS + display)
// - Service watchdog, Wi-Fi, and sealed-user controls (magnet)
// - Provide a lightweight runtime heartbeat for diagnostics
//
// Design notes:
// - Wi-Fi is OFF by default and only enabled via explicit user action
// - GPS logging is the primary operating mode
// - A single sealed Hall/reed switch (magnet) controls power + configuration
// - All hardware-specific initialization is delegated to managers
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

// ESP32 heap diagnostics
#include <esp_system.h>

// -----------------------------------------------------------------------------
// GLOBAL SYSTEM STATE
// -----------------------------------------------------------------------------

// Indicates whether the system is currently sleeping (deep sleep)
bool sleep_mode = false;

// Flag set during early boot to force shutdown (e.g. brown-out / reset)
extern bool reset_boot;

// -----------------------------------------------------------------------------
// SEALED USER INPUT (MAGNET / HALL SWITCH)
// -----------------------------------------------------------------------------

// GPIO connected to reed / Hall switch (input-only, RTC-capable)
static constexpr uint8_t  MAGNET_PIN     = 39;

// Gesture timing thresholds
static constexpr uint32_t TAP_MAX_MS     = 500;   // short tap → power toggle
static constexpr uint32_t WIFI_HOLD_MS   = 6000;  // long hold → Wi-Fi config
static constexpr uint32_t BOOT_IGNORE_MS = 1500;  // ignore magnet immediately after wake

// Debounce / stability thresholds
static constexpr uint32_t LOW_STABLE_MS      = 30;  // confirm magnet LOW
static constexpr uint32_t RELEASE_STABLE_MS  = 40;  // confirm magnet release

// Magnet gesture state (private to this file)
static uint32_t bootTime           = 0;
static uint32_t magnetPressTime    = 0;
static bool     magnetLongHandled  = false;

// -----------------------------------------------------------------------------
// FORWARD DECLARATIONS
// -----------------------------------------------------------------------------
static void startTasks();
static void heartbeat();
static const char* modeToString(SystemMode mode);
static void magnet_init();
static void magnet_poll();

// -----------------------------------------------------------------------------
// MAGNET INPUT HANDLING
// -----------------------------------------------------------------------------

// Initialise the sealed magnet input.
// Must be called once during setup().
static void magnet_init()
{
  // GPIO 39 has no internal pull-ups or pull-downs
  pinMode(MAGNET_PIN, INPUT);

  // Record boot time to suppress false triggers immediately after wake
  bootTime = millis();
}

// Poll magnet state and interpret user gestures.
// Called repeatedly from the main loop.
static void magnet_poll()
{
  const uint32_t now = millis();

  // Safety: never act on magnet immediately after boot/wake
  if (now - bootTime < BOOT_IGNORE_MS) return;

  // LOW is meaningful. HIGH is floating/noise.
  const bool rawLow = (digitalRead(MAGNET_PIN) == LOW);

  // Debounce LOW assertion
  static uint32_t lowSince = 0;
  if (rawLow) {
    if (lowSince == 0) lowSince = now;
  } else {
    lowSince = 0;
  }
  const bool active = (lowSince != 0) && (now - lowSince >= LOW_STABLE_MS);

  // Debounce release / inactivity (only matters after we were active)
  static uint32_t inactiveSince = 0;
  if (!active) {
    if (inactiveSince == 0) inactiveSince = now;
  } else {
    inactiveSince = 0;
  }
  const bool inactiveStable = (inactiveSince != 0) && (now - inactiveSince >= RELEASE_STABLE_MS);

  // Track edges based on "active" (not raw pin)
  static bool prevActive = false;

  // Press edge
  if (active && !prevActive) {
    magnetPressTime   = now;
    magnetLongHandled = false;
  }

  // Long hold → enter Wi-Fi configuration
  if (active && !magnetLongHandled && (now - magnetPressTime >= WIFI_HOLD_MS)) {
    magnetLongHandled = true;
    setMode(MODE_FIELD_CONFIG);
  }

  // Release edge (stable inactivity after previously active)
  if (!active && prevActive && inactiveStable) {
    const uint32_t held = now - magnetPressTime;

    // If we already consumed it as a long-hold, do nothing on release
    if (!magnetLongHandled && held <= TAP_MAX_MS) {
      setMode(getMode() == MODE_SLEEP ? MODE_LOGGING : MODE_SLEEP);
    }
  }

  prevActive = active;
}

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------
void setup()
{
  // ---------------------------------------------------------------------------
  // Early boot
  //
  // - Serial, battery ADC, SPI, system timing
  // - E-paper bring-up and boot screen
  // - Enforces shutdown on low battery or forced reset
  //
  // Must run before storage, config, GPS, or tasks.
  // ---------------------------------------------------------------------------
  initBoot();

  // ---------------------------------------------------------------------------
  // Storage
  //
  // - Mount SD card (optional)
  // - Mount LittleFS (mandatory)
  // - Perform basic I/O sanity checks
  //
  // Required before configuration loading or logging.
  // ---------------------------------------------------------------------------
  initStorage();

  // ---------------------------------------------------------------------------
  // Configuration
  //
  // - Load config.txt from LittleFS
  // - Create defaults if missing or invalid
  // - Apply derived runtime values (RTC, calibration, UI)
  // ---------------------------------------------------------------------------
  initConfig();

  // ---------------------------------------------------------------------------
  // GPS detection and configuration
  // ---------------------------------------------------------------------------
  initGPS();

  // ---------------------------------------------------------------------------
  // Sealed user input + default mode
  // ---------------------------------------------------------------------------
  magnet_init();

  // Default power-on behaviour:
  // - Start immediately in GPS logging mode
  // - Wi-Fi remains OFF unless explicitly requested
  setMode(MODE_LOGGING);

  // ---------------------------------------------------------------------------
  // Start core FreeRTOS tasks
  // ---------------------------------------------------------------------------
  startTasks();
}

// -----------------------------------------------------------------------------
// TASK STARTUP
// -----------------------------------------------------------------------------
static void startTasks()
{
  BaseType_t ok;

  // GPS task (core 1)
  ok = xTaskCreatePinnedToCore(taskOne, "TaskGPS", 10000, nullptr, 1, &t1, 1);
  if (ok != pdPASS) {
    Serial.println("[TASK   ] ERROR: TaskGPS create failed");
  }

  // Display task (core 0)
  ok = xTaskCreatePinnedToCore(taskTwo, "TaskDisplay", 10000, nullptr, 1, &t2, 0);
  if (ok != pdPASS) {
    Serial.println("[TASK   ] ERROR: TaskDisplay create failed");
  }

  // Guard: only query stack watermark if handle is valid
  Serial.print("[TASK   ] started");
  if (t1) { Serial.print(" t1_hw="); Serial.print(uxTaskGetStackHighWaterMark(t1)); }
  if (t2) { Serial.print(" t2_hw="); Serial.print(uxTaskGetStackHighWaterMark(t2)); }
  Serial.println();
}

// -----------------------------------------------------------------------------
// MAIN LOOP
// -----------------------------------------------------------------------------
void loop()
{
  // Handle sealed user input (magnet gestures)
  magnet_poll();

  // Feed watchdog
  watchdogLoop();

  const SystemMode mode = getMode();

  // Service Wi-Fi stack only when Wi-Fi is enabled by mode
  if (mode == MODE_HOME || mode == MODE_FIELD_CONFIG) {
    wifi_loop();
  }

  // Lightweight runtime diagnostics
  heartbeat();

  // Yield to FreeRTOS (loop is not time-critical)
  delay(10);
}

// -----------------------------------------------------------------------------
// HEARTBEAT (BRING-UP / DIAGNOSTICS)
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
    Serial.print(esp_get_minimum_free_heap_size());

    // Stack watermark is best-effort (only if tasks exist)
    if (t1) { Serial.print(" t1_hw="); Serial.print(uxTaskGetStackHighWaterMark(t1)); }
    if (t2) { Serial.print(" t2_hw="); Serial.print(uxTaskGetStackHighWaterMark(t2)); }

    Serial.println();
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
