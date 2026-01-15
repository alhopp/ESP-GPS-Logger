#include <Arduino.h>

#include "magnet_input.h"
#include "Definitions.h"
#include "system_mode.h"
#include "Globals.h"
#include "task_display.h"   // <-- REQUIRED

#include "Display/screen_system.h"

// -----------------------------------------------------------------------------
// Gesture thresholds
// -----------------------------------------------------------------------------
namespace {
constexpr uint32_t HALL_STABLE_MS = 20;
constexpr uint32_t SLEEP_HOLD_MS  = 1000;
constexpr uint32_t WIFI_HOLD_MS   = 4000;
constexpr uint32_t BOOT_IGNORE_MS = 1500;
}

// -----------------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------------
static uint32_t bootTime = 0;
static uint32_t pressTime = 0;
static bool     longHandled = false;
static bool     waitingForRelease = false;

// -----------------------------------------------------------------------------
// External state (exported)
// -----------------------------------------------------------------------------
bool magnet_active = false;

// -----------------------------------------------------------------------------
// Init
// -----------------------------------------------------------------------------
void magnet_init()
{
  pinMode(MAGNET_PIN, INPUT_PULLUP);
  bootTime = millis();

  if (woke_from_sleep) {
    waitingForRelease = true;
    pressTime = 0;
    longHandled = false;
  }
}

// -----------------------------------------------------------------------------
// Poll
// -----------------------------------------------------------------------------
void magnet_poll()
{
  const uint32_t now = millis();
  static bool prev_magnet_active = false;

  // ---------------------------------------------------------------------------
  // After deep sleep wake: ignore until released once
  // ---------------------------------------------------------------------------
  if (waitingForRelease) {
    if (digitalRead(MAGNET_PIN) == HIGH) {
      waitingForRelease = false;
      woke_from_sleep = false;
      bootTime = now;
    } else return;
  }

  // ---------------------------------------------------------------------------
  // Ignore input briefly after boot / wake
  // ---------------------------------------------------------------------------
  if (now - bootTime < BOOT_IGNORE_MS) return;

  // ---------------------------------------------------------------------------
  // Raw input (LOW = magnet present)
  // ---------------------------------------------------------------------------
  const bool rawActive = (digitalRead(MAGNET_PIN) == LOW);

  // ---------------------------------------------------------------------------
  // Stability filter
  // ---------------------------------------------------------------------------
  static bool active = false;
  static bool prevActive = false;
  static uint32_t stableSince = 0;

  if (rawActive != active) {
    if (!stableSince) stableSince = now;
    else if (now - stableSince >= HALL_STABLE_MS) {
      active = rawActive;
      stableSince = 0;
    }
  } else {
    stableSince = 0;
  }

  // ---------------------------------------------------------------------------
  // Export stable state
  // ---------------------------------------------------------------------------
  magnet_active = active;


  // ---------------------------------------------------------------------------
  // UI update on magnet state change (once per edge)
  // ---------------------------------------------------------------------------
  if (magnet_active != prev_magnet_active) {
    screen_request_magnet_affordance();
    prev_magnet_active = magnet_active;
  }
  // ---------------------------------------------------------------------------
  // Press edge
  // ---------------------------------------------------------------------------
  if (active && !prevActive) {
    pressTime = now;
    longHandled = false;
  }

  // ---------------------------------------------------------------------------
  // Long hold → CONFIG
  // ---------------------------------------------------------------------------
  if (active && !longHandled && (now - pressTime >= WIFI_HOLD_MS)) {
    longHandled = true;
    if (getMode() == MODE_IDLE) setMode(MODE_WIFI_SOFT_AP);
  }

  // Release → short press action or config exit
if (!active && prevActive && !longHandled) {
    const uint32_t held = now - pressTime;

    if (held >= SLEEP_HOLD_MS) {
        switch (getMode()) {
            case MODE_IDLE:
                setMode(MODE_WAIT_SATS);   // start logging
                break;

            case MODE_LOGGING:
            case MODE_WAIT_SATS:
                setMode(MODE_SLEEP);       // stop logging
                break;

            case MODE_WIFI_SOFT_AP:
                setMode(MODE_IDLE);        // exit config mode
                break;

            default:
                break;
        }
    }
}



  prevActive = active;
}
