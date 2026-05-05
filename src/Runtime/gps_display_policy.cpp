#include "Runtime/gps_display_policy.h"

#include <Arduino.h>

#include "Core/Globals.h"
#include "Core/system_mode.h"
#include "Display/display_geometry.h"
#include "Runtime/display_redraw.h"

namespace {
constexpr DisplayWindow SAT_WAIT_WINDOW = DISPLAY_BOTTOM_STATUS_WINDOW;
constexpr DisplayWindow SPEED_WINDOW = DISPLAY_FULL_WINDOW;

void updateSatelliteWaitDisplay(const GpsFix& fix)
{
  static uint8_t lastSV = 0;

  if (getMode() != MODE_WAIT_SATS) return;

  if (fix.satellites != lastSV) {
    lastSV = fix.satellites;
    screen_request_partial(SAT_WAIT_WINDOW);
  }
}

void updateSpeedDisplayThrottle(const GpsFix& fix)
{
  static uint32_t lastSpeedUpdateMs = 0;

  if (getMode() != MODE_LOGGING || !GPS_Signal_OK) return;

  const float kts = fix.speedKnots;
  const uint32_t intervalMs =
    kts < 10.0f ? UINT32_MAX :
    kts < 20.0f ? 5000 :
    kts < 38.0f ? 3000 : 1000;

  const uint32_t now = millis();
  if (intervalMs != UINT32_MAX && now - lastSpeedUpdateMs >= intervalMs) {
    lastSpeedUpdateMs = now;
    screen_request_partial(SPEED_WINDOW);
  }
}
}

void gps_display_policy_update(const GpsFix& fix)
{
  updateSatelliteWaitDisplay(fix);
  updateSpeedDisplayThrottle(fix);
}
