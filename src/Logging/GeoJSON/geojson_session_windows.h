#pragma once

// ============================================================================
// geojson_session_windows.h
//
// Shared predicates for GeoJSON export windows derived from session stats.
// ============================================================================

#include "Session/session_stats_snapshot.h"

inline bool geojson_has_window(const SessionWindow& window)
{
  return window.startSbp >= 1 && window.endSbp >= window.startSbp;
}
