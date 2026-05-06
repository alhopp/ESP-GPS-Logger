// ============================================================================
// screen_stats.h
//
// Stats screen renderer (paged).
//
// Public API:
// - draw_STATS(page): draw one stats page (1..11)
//
// Design rules:
// - draw_* functions draw only (no paging / no display.display())
// - No side-effects: no Wi-Fi/GPS toggles, no mode changes
// - The display task owns clear/refresh policy
// ============================================================================

#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Page range (keep in sync with screen_stats.cpp switch)
// -----------------------------------------------------------------------------
constexpr uint8_t STATS_PAGE_MIN = 1;
constexpr uint8_t STATS_PAGE_MAX = 11;

// -----------------------------------------------------------------------------
// Main entry point
// -----------------------------------------------------------------------------
void draw_STATS(uint8_t page);
