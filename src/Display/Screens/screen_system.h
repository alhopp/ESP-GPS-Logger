#pragma once

// -----------------------------------------------------------------------------
// screen_system.h
//
// System/status screen renderers.
//
// Responsibilities:
// - Declare stateless draw_*() functions for non-speed system modes
// - Keep the public screen API stable while implementations live in focused
//   per-mode .cpp files
//
// Design rules:
// - draw_*() functions ONLY render content
// - No paging (firstPage / nextPage)
// - No display.display() or fillScreen()
// - No hidden state or allocation
//
// Ownership:
// - task_display controls refresh / paging
// - system_mode controls which screen is active
// - screen_system_common.* owns private shared drawing helpers
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Public draw functions (called via screen dispatch)
// -----------------------------------------------------------------------------

// Early boot / low-battery screen
void draw_BOOT();

// Wait for Config or logging
void draw_IDLE();

// Configuration / Wi-Fi SoftAP screen
void draw_WIFI_CONFIG();

// Waiting-for-GPS-fix screen
void draw_WAIT_SATS();

// Shutdown / sleep summary screen
void draw_SLEEP();

// Request a partial redraw of the idle/config magnet affordance.
void screen_request_magnet_affordance();
