#include <Arduino.h>

#include "magnet_input.h"
#include "Definitions.h"
#include "system_mode.h"
#include "Globals.h"

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
// Init
// -----------------------------------------------------------------------------
void magnet_init()
{
  pinMode(MAGNET_PIN, INPUT_PULLUP);
  bootTime = millis();

  if (woke_from_sleep) {
    waitingForRelease = true;   // force clean release
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

  // ---------------------------------------------------------------------------
  // After deep sleep wake: ignore until released once
  // ---------------------------------------------------------------------------
  if (waitingForRelease) {
    if (digitalRead(MAGNET_PIN) == HIGH) {
      waitingForRelease = false;
      woke_from_sleep = false;
      bootTime = now;
    } else {
      return;
    }
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
  static bool     active = false;
  static bool     prevActive = false;   // ✅ RESTORED
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
  // Press edge
  // ---------------------------------------------------------------------------
  if (active && !prevActive) {
    pressTime   = now;
    longHandled = false;
  }

  // ---------------------------------------------------------------------------
  // Long hold → CONFIG (fires while still pressed)
  // ---------------------------------------------------------------------------
  if (active && !longHandled && (now - pressTime >= WIFI_HOLD_MS)) {
    longHandled = true;

    if (getMode() == MODE_IDLE) {
      setMode(MODE_WIFI_SOFT_AP);
    }
  }

  // ---------------------------------------------------------------------------
  // Release → short press → START
  // ---------------------------------------------------------------------------
  if (!active && prevActive && !longHandled) {
    const uint32_t held = now - pressTime;

    if (held >= SLEEP_HOLD_MS) {
      if (getMode() == MODE_IDLE) {
        setMode(MODE_WAIT_SATS);
      }
    }
  }

  prevActive = active;   // ✅ CRITICAL
}
