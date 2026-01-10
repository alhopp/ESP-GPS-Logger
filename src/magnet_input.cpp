#include <Arduino.h>

#include "magnet_input.h"

#include "Definitions.h"
#include "system_mode.h"

// -----------------------------------------------------------------------------
// Gesture thresholds ms
// -----------------------------------------------------------------------------
namespace {

constexpr uint32_t HALL_STABLE_MS   = 20;    // Hall stability filter (10–30 ms)
constexpr uint32_t SLEEP_HOLD_MS    = 1000;  // Short hold → start / stop
constexpr uint32_t WIFI_HOLD_MS     = 4000;  // Long hold → CONFIG
constexpr uint32_t BOOT_IGNORE_MS   = 1500;  // Ignore input just after boot

} // anonymous namespace

// -----------------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------------

static uint32_t bootTime    = 0;
static uint32_t pressTime   = 0;
static bool     longHandled = false;

// -----------------------------------------------------------------------------
// MAGNET INIT
//
// Initialise the digital Hall sensor used as the sealed user control.
// Records boot time so magnet input can be ignored during startup transients.
// -----------------------------------------------------------------------------
void magnet_init()
{
  pinMode(MAGNET_PIN, INPUT);
  bootTime = millis();
}

// -----------------------------------------------------------------------------
// MAGNET POLL (Hall switch optimised)
//
// Authoritative interpreter of *user intent* via a digital Hall sensor.
//
// Hall sensors are solid-state (no contact bounce), so this implementation uses
// a lightweight stability filter rather than mechanical debounce.
//
// Gesture rules (authoritative):
//   - < SLEEP_HOLD_MS
//       * ignored
//   - ≥ SLEEP_HOLD_MS and < WIFI_HOLD_MS
//       * MODE_SLEEP              → start session (WAIT_SATS)
//       * MODE_WAIT_SATS / LOGGING → stop session (SLEEP)
//   - ≥ WIFI_HOLD_MS
//       * MODE_SLEEP              → enter CONFIG (Wi-Fi AP)
//       * all other modes         → ignored
//
// IMPORTANT:
//   - No toggle semantics
//   - No direct side-effects
//   - All behaviour changes flow through setMode()
// -----------------------------------------------------------------------------
void magnet_poll()
{
  const uint32_t now = millis();

  // ---------------------------------------------------------------------------
  // Ignore magnet input immediately after boot / wake
  // ---------------------------------------------------------------------------
  if (now - bootTime < BOOT_IGNORE_MS) return;

  // ---------------------------------------------------------------------------
  // Raw Hall sensor input
  // LOW = magnetic field present
  // ---------------------------------------------------------------------------
  const bool rawActive = (digitalRead(MAGNET_PIN) == LOW);

  // ---------------------------------------------------------------------------
  // Hall stability filter
  // Require the raw state to be stable for HALL_STABLE_MS
  // ---------------------------------------------------------------------------
  static bool     active      = false;   // filtered / qualified state
  static uint32_t stableSince = 0;

  if (rawActive != active) {
    if (stableSince == 0) {
      stableSince = now;
    } else if (now - stableSince >= HALL_STABLE_MS) {
      active = rawActive;
      stableSince = 0;
    }
  } else {
    stableSince = 0;
  }

  static bool prevActive = false;

  // ---------------------------------------------------------------------------
  // PRESS EDGE
  // Capture the moment a qualified press begins
  // ---------------------------------------------------------------------------
  if (active && !prevActive) {
    pressTime   = now;
    longHandled = false;
  }

  // ---------------------------------------------------------------------------
  // LONG HOLD (≥ WIFI_HOLD_MS)
  // Intent: Enter CONFIG (only from sleep)
  // ---------------------------------------------------------------------------
  if (active &&
      !longHandled &&
      (now - pressTime >= WIFI_HOLD_MS)) {

    longHandled = true;

    if (getMode() == MODE_SLEEP) {
      LOG_SYS("INTENT", "Sleep → Config");
      setMode(MODE_WIFI_SOFT_AP);
    }
  }

  // ---------------------------------------------------------------------------
  // RELEASE (≥ SLEEP_HOLD_MS, < WIFI_HOLD_MS)
  // Intent: Start or stop session
  // ---------------------------------------------------------------------------
  if (!active && prevActive && !longHandled) {

    const uint32_t held = now - pressTime;

    if (held >= SLEEP_HOLD_MS && held < WIFI_HOLD_MS) {

      // ---- START SESSION ----
      if (getMode() == MODE_SLEEP) {
        LOG_SYS("INTENT", "Sleep → Start");
        setMode(MODE_WAIT_SATS);
      }

      // ---- STOP SESSION ----
      else if (getMode() == MODE_WAIT_SATS ||
               getMode() == MODE_LOGGING) {
        LOG_SYS("INTENT", "Session → Sleep");
        setMode(MODE_SLEEP);
      }

      // MODE_WIFI_SOFT_AP (CONFIG):
      // Intentionally ignored — exit via UI only
    }
  }

  prevActive = active;
}
