// ============================================================================
// geojson_session_export.cpp
//
// Converts a completed SBP log into GeoJSON. Adds track points, derived session
// features, graph series, and static system/session metadata.
// ============================================================================

#include "Logging/GeoJSON/geojson_session_export.h"

#include <Arduino.h>

#include "Core/log.h"
#include "Core/system_info.h"
#include "Logging/GeoJSON/geojson_export_limits.h"
#include "Logging/GeoJSON/geojson_graphs.h"
#include "Logging/GeoJSON/geojson_sbp_reader.h"
#include "Logging/GeoJSON/geojson_writer.h"
#include "Session/session_stats_snapshot.h"

namespace {
bool hasWindow(const SessionWindow& window)
{
  return window.startSbp >= 1 && window.endSbp >= window.startSbp;
}

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

void addSbpRangePoints(const char* sbpPath, int startGpsIdx, int endGpsIdx, int step)
{
  if (startGpsIdx < 0 || endGpsIdx < startGpsIdx) return;
  if (step < 1) step = 1;

  File file;
  if (!geojson_sbp_open(file, sbpPath)) return;

  GeoJsonSbpFrame frame;
  int gpsIndex = 1;
  while (geojson_sbp_read_frame(file, frame)) {
    if (gpsIndex >= startGpsIdx && gpsIndex <= endGpsIdx &&
        ((gpsIndex - startGpsIdx) % step) == 0) {
      geojson_add_point(geojson_sbp_frame_lat(frame), geojson_sbp_frame_lon(frame));
    }
    if (gpsIndex > endGpsIdx) break;
    gpsIndex++;
  }

  file.close();
}

void addRangeFeature(const char* sbpPath, const char* mode, int startGpsIdx, int endGpsIdx)
{
  if (startGpsIdx < 0 || endGpsIdx < startGpsIdx) return;

  const int samples = endGpsIdx - startGpsIdx + 1;
  int step = systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 1;
  if (samples / step > GEOJSON_MAX_SERIES_POINTS) {
    step = (samples + GEOJSON_MAX_SERIES_POINTS - 1) / GEOJSON_MAX_SERIES_POINTS;
  }

  geojson_begin_feature(mode);
  addSbpRangePoints(sbpPath, startGpsIdx, endGpsIdx, step);
  geojson_end_feature();
}

void addOneHourFeature(const char* sbpPath, const SessionStatsSnapshot& snapshot)
{
  if (!hasWindow(snapshot.oneHour)) return;

  const int sampleRate = systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 1;
  const int samples = snapshot.oneHour.endSbp - snapshot.oneHour.startSbp + 1;
  const int seconds = samples / sampleRate;
  int secStep = seconds > GEOJSON_MAX_SERIES_POINTS
      ? (seconds + GEOJSON_MAX_SERIES_POINTS - 1) / GEOJSON_MAX_SERIES_POINTS
      : 1;

  geojson_begin_feature("1h");
  addSbpRangePoints(sbpPath, snapshot.oneHour.startSbp, snapshot.oneHour.endSbp, secStep * sampleRate);
  geojson_end_feature();
}

void addDerivedFeatures(const char* sbpPath, const SessionStatsSnapshot& snapshot)
{
  addRangeFeature(sbpPath, "2s", snapshot.max2s.startSbp, snapshot.max2s.endSbp);

  for (int i = 0; i < 5; i++) {
    addRangeFeature(sbpPath, "10s", snapshot.tenSecond[i].startSbp, snapshot.tenSecond[i].endSbp);
  }

  addRangeFeature(sbpPath, "alpha", snapshot.alpha.startSbp, snapshot.alpha.endSbp);
  addRangeFeature(sbpPath, "nm", snapshot.nauticalMile.startSbp, snapshot.nauticalMile.endSbp);
  addOneHourFeature(sbpPath, snapshot);
}

bool addBaseTrackFromSbp(const char* sbpPath)
{
  File file;
  if (!geojson_sbp_open(file, sbpPath)) return false;

  GeoJsonSbpFrame frame;
  while (geojson_sbp_read_frame(file, frame)) {
    geojson_add_track_point(geojson_sbp_frame_lat(frame), geojson_sbp_frame_lon(frame));
  }

  file.close();
  return true;
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
  if (!addBaseTrackFromSbp(sbpPath)) {
    geojson_end_feature();
    geojson_end();
    return false;
  }

  attachSessionStats(snapshot);
  geojson_attach_graph_series(sbpPath, snapshot);
  geojson_end_feature();
  addDerivedFeatures(sbpPath, snapshot);
  geojson_end();
  return true;
}
