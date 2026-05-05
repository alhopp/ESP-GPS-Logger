#pragma once

#include <Arduino.h>

#include <GxEPD2_BW.h>
#include <epd/GxEPD2_213_B74.h>

#include "Core/board_pins.h"

extern GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display;

// ============================================================================
//  Public E-paper API
// ============================================================================

void display_init();
