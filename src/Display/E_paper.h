#pragma once

#include <Arduino.h>

#include <GxEPD2_BW.h>
#include <epd/GxEPD2_213_B74.h>

extern GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display;

// ==========================
// E-paper pin mapping
// ==========================
#define ELINK_SS     5
#define ELINK_DC     17
#define ELINK_RESET  16
#define ELINK_BUSY  4

// ============================================================================
//  Public E-paper API
// ============================================================================

void display_init();
