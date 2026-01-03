#pragma once

// ======================================================
// screen_draw.h
// ------------------------------------------------------
// Screen render entry points
// ======================================================
#include "system_mode.h"

void draw_BOOT();
void draw_WIFI_SOFT_AP();
void draw_WIFI_STATION();
void draw_SLEEP();

void draw_GPS_INIT();

void draw_WIFI_ON();


void draw_SPEED();

void draw_STATS1();
void draw_STATS2();
void draw_STATS3();
void draw_STATS4();
void draw_STATS5();
void draw_STATS6();
void draw_STATS7();
void draw_STATS8();
void draw_STATS9();
void draw_STATSA();
void draw_STATSB();

using DrawFn = void (*)();
extern const DrawFn ScreenDrawTable[];

void drawTopLeftTitle(const char* msg);
int  device_boot_log(int rows, int ws = 0);



// -----------------------------------------------------------------------------
// MODE → DRAW FUNCTION (authoritative)
// -----------------------------------------------------------------------------
DrawFn getDrawFnForMode(SystemMode mode);
