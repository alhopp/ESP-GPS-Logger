// ============================================================================
// screen_stats.h
//
// Stats screen renderer (paged).
//
// Design:
// - Exposes ONE entry point: draw_STATS(page)
// - Individual pages are implemented internally (switch)
// - No side effects: drawing only
// - No dynamic allocation
// ============================================================================

#pragma once

#include <stdint.h>

// Total number of stats pages (1..11 where 11 == old 'B')
constexpr uint8_t STATS_PAGE_MIN = 1;
constexpr uint8_t STATS_PAGE_MAX = 11;

// Draw the requested stats page (1..11).
// Page mapping:
//  1  = old draw_STATS1
//  2  = old draw_STATS2
//  3  = old draw_STATS3
//  4  = old draw_STATS4
//  5  = old draw_STATS5
//  6  = old draw_STATS6
//  7  = old draw_STATS7
//  8  = old draw_STATS8
//  9  = old draw_STATS9
// 10  = old draw_STATSA
// 11  = old draw_STATSB
void draw_STATS(uint8_t page);
