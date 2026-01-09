#pragma once

// -----------------------------------------------------------------------------
// screen_system.h
//
// System / status screens.
//
// Responsibilities:
// - Declare stateless draw_*() functions for system-related modes
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
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Public draw functions (called via screen dispatch)
// -----------------------------------------------------------------------------

// Early boot / low-battery screen
void draw_BOOT();

// Configuration / Wi-Fi SoftAP screen
void draw_WIFI_SOFT_AP();

// Waiting-for-GPS-fix screen
void draw_WAIT_SATS();

// Shutdown / sleep summary screen
void draw_SLEEP();
