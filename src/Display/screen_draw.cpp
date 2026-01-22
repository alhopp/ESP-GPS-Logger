// -----------------------------------------------------------------------------
// screen_draw.cpp
//
// Screen rendering dispatch layer.
//
// Responsibilities:
// - Provide stateless draw functions
// - Map authoritative SystemMode → draw function
//
// Design rules:
// - No state, no caching, no side-effects
// - No mode inference or Wi-Fi logic
// - task_display decides *when* to draw
// - system_mode decides *what mode we are in*
// -----------------------------------------------------------------------------

#include "Display/screen_draw.h"
#include "Display/screen_speed.h"
#include "Display/screen_system.h"
#include "system_mode.h"

// -----------------------------------------------------------------------------
// MODE → DRAW FUNCTION (authoritative)
// -----------------------------------------------------------------------------
DrawFn getDrawFnForMode(SystemMode mode)
{
  switch (mode) {
    case MODE_BOOT:          return draw_BOOT;
    case MODE_IDLE:          return draw_IDLE;
    case MODE_WAIT_SATS:     return draw_WAIT_SATS;
    case MODE_CONFIG:  return draw_WIFI_SOFT_AP;
    case MODE_LOGGING:       return draw_SPEED;
    case MODE_SLEEP:         return draw_SLEEP;
    default:                 return nullptr;
  }

}

