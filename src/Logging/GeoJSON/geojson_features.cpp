// ============================================================================
// geojson_features.cpp
//
// Builds GeoJSON LineString features from SBP frame ranges: the full base track
// plus highlighted derived result windows such as 2s, 10s, alpha, NM, and 1h.
// ============================================================================

#include "Logging/GeoJSON/geojson_features.h"

#include <Arduino.h>

#include "Core/system_info.h"
#include "Logging/GeoJSON/geojson_writer.h"
#include "Logging/SBP/sbp_session_reader.h"
#include "Session/session_stats_snapshot.h"

namespace {
constexpr int GEOJSON_MAX_SERIES_POINTS = 240;
constexpr int BASE_TRACK_1HZ_THRESHOLD_SECONDS = 20 * 60;

bool hasWindow(const SessionWindow& window)
{
  return window.startSbp >= 1 && window.endSbp >= window.startSbp;
}

struct DerivedFeature {
  const char* mode;
  int startSbp;
  int endSbp;
};

void addSbpRangePoints(const char* sbpPath, int startGpsIdx, int endGpsIdx, int step)
{
  if (startGpsIdx < 0 || endGpsIdx < startGpsIdx) return;
  if (step < 1) step = 1;

  File file;
  if (!sbp_session_open(file, sbpPath)) return;

  SbpFrame frame;
  int firstGpsIdx = startGpsIdx < 1 ? 1 : startGpsIdx;
  const int offset = (firstGpsIdx - startGpsIdx) % step;
  if (offset != 0) {
    firstGpsIdx += step - offset;
  }

  for (int gpsIndex = firstGpsIdx; gpsIndex <= endGpsIdx; gpsIndex += step) {
    if (sbp_session_read_frame_at(file, gpsIndex, frame)) {
      geojson_add_point(sbp_frame_lat(frame), sbp_frame_lon(frame));
    }
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

int baseTrackStride(int frameCount)
{
  const int sampleRate = systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 1;
  const int thresholdFrames = BASE_TRACK_1HZ_THRESHOLD_SECONDS * sampleRate;
  return frameCount > thresholdFrames ? sampleRate : 1;
}
}

bool geojson_add_base_track_from_sbp(const char* sbpPath)
{
  const int frameCount = sbp_session_count_frames(sbpPath);
  if (frameCount <= 0) return false;

  File file;
  if (!sbp_session_open(file, sbpPath)) return false;

  SbpFrame frame;
  const int stride = baseTrackStride(frameCount);
  int gpsIndex = 1;

  while (sbp_session_read_frame(file, frame)) {
    if ((gpsIndex - 1) % stride == 0) {
      geojson_add_track_point(sbp_frame_lat(frame), sbp_frame_lon(frame));
    }
    gpsIndex++;
  }

  if ((frameCount - 1) % stride != 0 &&
      sbp_session_read_frame_at(file, frameCount, frame)) {
    geojson_add_track_point(sbp_frame_lat(frame), sbp_frame_lon(frame));
  }

  file.close();
  return true;
}

void geojson_add_derived_features(const char* sbpPath, const SessionStatsSnapshot& snapshot)
{
  const DerivedFeature features[] = {
    { "2s", snapshot.max2s.startSbp, snapshot.max2s.endSbp },
    { "10s", snapshot.tenSecond[0].startSbp, snapshot.tenSecond[0].endSbp },
    { "10s", snapshot.tenSecond[1].startSbp, snapshot.tenSecond[1].endSbp },
    { "10s", snapshot.tenSecond[2].startSbp, snapshot.tenSecond[2].endSbp },
    { "10s", snapshot.tenSecond[3].startSbp, snapshot.tenSecond[3].endSbp },
    { "10s", snapshot.tenSecond[4].startSbp, snapshot.tenSecond[4].endSbp },
    { "alpha", snapshot.alpha.startSbp, snapshot.alpha.endSbp },
    { "nm", snapshot.nauticalMile.startSbp, snapshot.nauticalMile.endSbp }
  };

  for (const DerivedFeature& feature : features) {
    addRangeFeature(sbpPath, feature.mode, feature.startSbp, feature.endSbp);
  }

  addOneHourFeature(sbpPath, snapshot);
}
