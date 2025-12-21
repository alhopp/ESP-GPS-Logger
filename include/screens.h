#pragma once

// ======================================================
// screens.h
// ------------------------------------------------------
// Screen identity & current screen state
// NO drawing logic
// ======================================================

enum ScreenID : uint8_t {
  SCREEN_BOOT,
  SCREEN_GPS_INIT,

  SCREEN_WIFI_ON,
  SCREEN_WIFI_STATION,
  SCREEN_WIFI_SOFT_AP,

  SCREEN_SPEED,

  SCREEN_STATS1,
  SCREEN_STATS2,
  SCREEN_STATS3,
  SCREEN_STATS4,
  SCREEN_STATS5,
  SCREEN_STATS6,
  SCREEN_STATS7,
  SCREEN_STATS8,
  SCREEN_STATS9,
  SCREEN_STATSA,
  SCREEN_STATSB
};

extern ScreenID screen;

