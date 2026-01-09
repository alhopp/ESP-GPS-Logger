// screen_context.h
#pragma once

#include <Arduino.h>
#include "Display/E_paper.h"
#include "Definitions.h"
#include <stdint.h>

// ---- Display ----
extern int16_t displayWidth;
extern int16_t displayHeight;

// ---- Fonts / Layout ----
#include "Fonts.h"

// ---- UI constants ----
extern float calibration_speed;
extern int run_count;

// ---- GPS / Stats data ----
#include "GPS/GPS_data.h"
#include "Ublox/ublox.h"


