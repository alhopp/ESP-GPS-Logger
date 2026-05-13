// ============================================================================
// geojson_session_export.cpp
//
// Converts a completed SBP log into GeoJSON. Adds track points, derived session
// features, graph series, and static system/session metadata.
// ============================================================================

#include "Logging/GeoJSON/geojson_session_export.h"

#include <Arduino.h>

#include "Core/log.h"
#include "Logging/GeoJSON/geojson_features.h"
#include "Logging/GeoJSON/geojson_graphs.h"
#include "Logging/GeoJSON/geojson_writer.h"
#include "Session/session_stats_snapshot.h"

namespace {
void attachSessionStats(const SessionStatsSnapshot& snapshot)
{
  GeoJSONStats s {
    .nm = snapshot.nauticalMile.speedKnots,
    .alpha = snapshot.alpha.speedKnots,
    .alphaDistance = static_cast<float>(snapshot.alpha.distanceM),
    .alphaClosure = snapshot.alpha.closureM,
    .h1 = snapshot.oneHour.speedKnots,
    .max = snapshot.max2s.speedKnots,
    .avg10 = snapshot.tenSecondAverageKnots,
    .r10 = {
      snapshot.tenSecond[0].speedKnots,
      snapshot.tenSecond[1].speedKnots,
      snapshot.tenSecond[2].speedKnots,
      snapshot.tenSecond[3].speedKnots,
      snapshot.tenSecond[4].speedKnots
    },
    .distance = snapshot.distanceKm
  };

  geojson_set_stats(s);
}
}

bool geojson_session_export_finalize(const char* sbpPath, const char* geojsonPath)
{
  if (!sbpPath || !geojsonPath) return false;
  const SessionStatsSnapshot snapshot = build_session_stats_snapshot();
  return geojson_session_export_finalize(sbpPath, geojsonPath, snapshot);
}

bool geojson_session_export_finalize(const char* sbpPath,
                                     const char* geojsonPath,
                                     const SessionStatsSnapshot& snapshot)
{
  if (!sbpPath || !geojsonPath) return false;

  if (!geojson_begin(geojsonPath)) {
    LOG_ERROR("STORAGE", "GeoJSON open failed");
    return false;
  }

  geojson_begin_feature("track");
  if (!geojson_add_base_track_from_sbp(sbpPath)) {
    geojson_end_feature();
    geojson_end();
    return false;
  }

  attachSessionStats(snapshot);
  geojson_attach_graph_series(sbpPath, snapshot);
  geojson_end_feature();
  geojson_add_derived_features(sbpPath, snapshot);
  geojson_end();
  return true;
}
