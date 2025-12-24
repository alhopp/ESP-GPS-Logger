// screen_context.h
#pragma once

#include <Arduino.h>
#include "E_paper.h"
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
#include "GPS_data.h"
#include "Ublox.h"

// Expose ONLY the instances screens need
extern GPS_time   S10;
extern GPS_time   S2;
extern GPS_speed  M100;
extern GPS_speed  M250;
extern GPS_speed  M500;
extern GPS_speed  M1852;
extern Alfa_speed A500;

// ---- UI helpers ----
void Stats_2s_3_lines(
    const char* l1,
    const char* l2,
    const char* l3,
    float v1,
    float v2,
    float v3
);

void Stats_4lines(
    const char* l1,
    const char* l2,
    const char* l3,
    const char* l4,
    float v1,
    float v2,
    float v3,
    float v4
);
