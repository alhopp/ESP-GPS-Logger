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

#include "screen_draw.h"
#include "screen_speed.h"
#include "system_mode.h"

#include "Fonts.h"
#include "Layout.h"
#include "E_paper.h"

// -----------------------------------------------------------------------------
// LEGACY DRAW TABLE (optional / retained for compatibility)
//
// NOTE:
// - This table is no longer authoritative for rendering decisions
// - It may still be used by legacy code paths (stats paging, etc.)
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MODE → DRAW FUNCTION (authoritative)
// -----------------------------------------------------------------------------
DrawFn getDrawFnForMode(SystemMode mode)
{
  switch (mode) {
    case MODE_BOOT:          return draw_BOOT;
    case MODE_WAIT_SATS:     return draw_WAIT_SATS;
    case MODE_WIFI_SOFT_AP:  return draw_WIFI_SOFT_AP;
    case MODE_WIFI_STATION:  return draw_WIFI_STATION;
    case MODE_LOGGING:       return draw_SPEED;
    case MODE_SLEEP:         return draw_SLEEP;
    default:                 return nullptr;
  }
}


// -----------------------------------------------------------------------------
// SHARED UI HELPERS
// -----------------------------------------------------------------------------
void drawTopLeftTitle(const char* msg)
{
  display.setFont(Fonts::Body9);
  display.setCursor(0, 0);
  display.print(msg);
}
