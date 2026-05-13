// ============================================================================
// geojson_features.cpp
//
// Builds GeoJSON LineString features from SBP frame ranges: the full base track
// plus highlighted derived result windows such as 2s, 10s, alpha, NM, and 1h.
// ============================================================================

#include "Logging/GeoJSON/geojson_features.h"

#include <Arduino.h>

#include "Core/system_info.h"
#include "Logging/GeoJSON/geojson_sbp_reader.h"
#include "Logging/GeoJSON/geojson_writer.h"
#include "Session/session_stats_snapshot.h"

namespace {
constexpr int GEOJSON_MAX_SERIES_POINTS = 240;

bool hasWindow(const SessionWindow& window)
{
  return window.startSbp >= 1 && window.endSbp >= window.startSbp;
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
}

bool geojson_add_base_track_from_sbp(const char* sbpPath)
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

void geojson_add_derived_features(const char* sbpPath, const SessionStatsSnapshot& snapshot)
{
  addRangeFeature(sbpPath, "2s", snapshot.max2s.startSbp, snapshot.max2s.endSbp);

  for (int i = 0; i < 5; i++) {
    addRangeFeature(sbpPath, "10s", snapshot.tenSecond[i].startSbp, snapshot.tenSecond[i].endSbp);
  }

  addRangeFeature(sbpPath, "alpha", snapshot.alpha.startSbp, snapshot.alpha.endSbp);
  addRangeFeature(sbpPath, "nm", snapshot.nauticalMile.startSbp, snapshot.nauticalMile.endSbp);
  addOneHourFeature(sbpPath, snapshot);
}
