#pragma once

#include <stdint.h>

// ============================================================================
// SessionStatsSnapshot
//
// Final, read-only view of a completed GPS session.
//
// This is a boundary object: metric calculators still own the maths, but
// display, serial debug, web export, and file export can all consume one
// consistent result set instead of each reading a different mix of globals.
// ============================================================================

struct SessionWindow {
  float speedKnots = 0.0f;
  int startSbp = -1;
  int endSbp = -1;
  int run = -1;
  uint32_t sumCms = 0;
};

struct SessionAlphaWindow {
  float speedKnots = 0.0f;
  int startSbp = -1;
  int endSbp = -1;
  int distanceM = 0;
  float closureM = 0.0f;
};

struct SessionStatsSnapshot {
  SessionWindow max2s;

  SessionWindow tenSecond[5];
  float tenSecondAverageKnots = 0.0f;
  int runCount = 0;

  SessionWindow nauticalMile;
  SessionAlphaWindow alpha;

  SessionWindow oneHour;

  float distanceKm = 0.0f;
  int frameCount = 0;
};

SessionStatsSnapshot build_session_stats_snapshot();
