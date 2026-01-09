#pragma once

// ======================================================
// screen_draw.h
// ------------------------------------------------------
// Mode-level screen render entry points ONLY
//
// These functions are the ONLY ones that may be returned
// by getDrawFnForMode(). All sub-screens and pages are
// internal details handled elsewhere.
// ======================================================

#include "system_mode.h"

// -----------------------------------------------------------------------------
// Mode-level screens (authoritative)
// -----------------------------------------------------------------------------


void draw_BOOT();
void draw_WAIT_SATS();
void draw_LOGGING();        // May internally show SPEED / STATS pages
void draw_WIFI_SOFT_AP();
void draw_SLEEP();



// -----------------------------------------------------------------------------
// Draw function type
// -----------------------------------------------------------------------------
using DrawFn = void (*)();

// -----------------------------------------------------------------------------
// Mode → draw function dispatch
// -----------------------------------------------------------------------------
DrawFn getDrawFnForMode(SystemMode mode);

// -----------------------------------------------------------------------------
// Shared UI helpers (NOT screens)
// -----------------------------------------------------------------------------
void drawTopLeftTitle(const char* msg);
int  device_boot_log(int rows, int ws = 0);

// -----------------------------------------------------------------------------
// Logo helpers (UI primitives, NOT screens)
// -----------------------------------------------------------------------------

// Draw board logo selected in config (no-op if disabled)
void drawBoardLogo(int16_t x, int16_t y);

// Draw sail logo selected in config (no-op if disabled)
void drawSailLogo(int16_t x, int16_t y);