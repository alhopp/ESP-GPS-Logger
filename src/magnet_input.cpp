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
static uint32_t bootTime    = 0;
static uint32_t pressTime   = 0;
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


  static uint32_t lastLog = 0;
  if (millis() - lastLog > 500) {
  LOG_SYS("MAG", "poll alive mode=%s", modeToString(getMode()));
  lastLog = millis();
}

  // ---------------------------------------------------------------------------
  // After deep sleep wake:
  // ignore magnet until it is released once
  // ---------------------------------------------------------------------------
  if (waitingForRelease) {
  if (digitalRead(MAGNET_PIN) == HIGH) {
    waitingForRelease = false;
    woke_from_sleep = false;
    bootTime = now;
    // DO NOT return — allow normal processing to resume
  } else {
    return; // still held → ignore
  }
}

static int lastRaw = -1;
int raw = digitalRead(MAGNET_PIN);
if (raw != lastRaw) {
  LOG_SYS("MAG", "raw=%d", raw);
  lastRaw = raw;
}


  // ---------------------------------------------------------------------------
  // Ignore input briefly after cold boot / reset
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


static bool lastActive = false;
if (active != lastActive) {
LOG_SYS("MAG", "stable active=%d", active);
lastActive = active;
}

  static bool prevActive = false;

  // ---------------------------------------------------------------------------
  // PRESS edge
  // ---------------------------------------------------------------------------
  if (active && !prevActive) {
    pressTime   = now;
    longHandled = false;
    LOG_SYS("MAG", "PRESS edge t=%lu", pressTime);
  }

  // ---------------------------------------------------------------------------
  // LONG HOLD → CONFIG (fires while still pressed)
  // ---------------------------------------------------------------------------
  if (active &&
      !longHandled &&
      (now - pressTime >= WIFI_HOLD_MS)) {

    longHandled = true;

    LOG_SYS("MAG", "LONG HOLD fired after %lu ms", now - pressTime);

    if (getMode() == MODE_SLEEP) {
      LOG_SYS("INTENT", "Sleep → Config");
      setMode(MODE_WIFI_SOFT_AP);
    }
  }

  // ---------------------------------------------------------------------------
  // RELEASE → short press start / stop
  // ---------------------------------------------------------------------------
  if (!active && prevActive && !longHandled) {

    const uint32_t held = now - pressTime;

    LOG_SYS("MAG", "RELEASE held=%lu ms mode=%s",
            held, modeToString(getMode()));

    if (held >= SLEEP_HOLD_MS) {

      if (getMode() == MODE_SLEEP) {
        LOG_SYS("INTENT", "Sleep → Start");
        setMode(MODE_WAIT_SATS);
      }
      else if (getMode() == MODE_WAIT_SATS ||
               getMode() == MODE_LOGGING) {
        LOG_SYS("INTENT", "Session → Sleep");
        setMode(MODE_SLEEP);
      }
    }
  }

  prevActive = active;
}
