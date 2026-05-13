// ============================================================================
// geojson_session_export.cpp
//
// Converts a completed SBP log into GeoJSON. Adds track points, derived session
// features, graph series, and static system/session metadata.
// ============================================================================

#include "Logging/geojson_session_export.h"

#include <Arduino.h>

#include "Core/log.h"
#include "Core/system_info.h"
#include "GPS/gps_config.h"
#include "Logging/geojson_sbp_reader.h"
#include "Logging/geojson_writer.h"
#include "Session/session_stats_snapshot.h"

namespace {
constexpr int GRAPH_MAX_POINTS = 240;

float graph2s[GRAPH_MAX_POINTS];
float graph10s[5][GRAPH_MAX_POINTS];
int graph10sCount[5];
float graphAlpha[GRAPH_MAX_POINTS];
float graphNm[GRAPH_MAX_POINTS];
float graph1h[GRAPH_MAX_POINTS];
float graphDistance[GRAPH_MAX_POINTS];
float graph1hMinutes = 0.0f;
float graphSessionMinutes = 0.0f;

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

void compressGraph(float* values, int& count, int& strideSamples)
{
  int write = 0;
  for (int read = 0; read < count; read += 2) {
    values[write++] = values[read];
  }
  count = write;
  strideSamples *= 2;
}

void appendCompactGraphPoint(float* values, int& count, int& strideSamples, float value)
{
  if (count >= GRAPH_MAX_POINTS) {
    compressGraph(values, count, strideSamples);
  }
  values[count++] = value;
}

int readGpsSpeedGraph(const char* sbpPath, float* out, int startGpsIdx, int endGpsIdx)
{
  if (!out || startGpsIdx < 0 || endGpsIdx < startGpsIdx) return 0;

  const int samples = endGpsIdx - startGpsIdx + 1;
  const int step = samples > GRAPH_MAX_POINTS ? (samples + GRAPH_MAX_POINTS - 1) / GRAPH_MAX_POINTS : 1;

  File file;
  if (!geojson_sbp_open(file, sbpPath)) return 0;

  GeoJsonSbpFrame frame;
  int gpsIndex = 1;
  int count = 0;
  while (geojson_sbp_read_frame(file, frame) && count < GRAPH_MAX_POINTS) {
    if (gpsIndex >= startGpsIdx && gpsIndex <= endGpsIdx &&
        ((gpsIndex - startGpsIdx) % step) == 0) {
      out[count++] = geojson_sbp_frame_knots(frame);
    }
    if (gpsIndex > endGpsIdx) break;
    gpsIndex++;
  }

  file.close();
  return count;
}

int readSecondSpeedGraph(const char* sbpPath, float* out, int startSecIdx, int endSecIdx)
{
  graph1hMinutes = 0.0f;
  if (!out || startSecIdx < 0 || endSecIdx < startSecIdx) return 0;

  const int sampleRate = systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 1;
  const int startGpsIdx = (startSecIdx * sampleRate) + 1;
  const int endGpsIdx = (endSecIdx + 1) * sampleRate;
  const int seconds = endSecIdx - startSecIdx + 1;
  const int secStep = seconds > GRAPH_MAX_POINTS ? (seconds + GRAPH_MAX_POINTS - 1) / GRAPH_MAX_POINTS : 1;

  File file;
  if (!geojson_sbp_open(file, sbpPath)) return 0;

  GeoJsonSbpFrame frame;
  int gpsIndex = 1;
  int secIndex = startSecIdx;
  int count = 0;
  uint32_t sumCms = 0;
  int samplesInSecond = 0;
  int secondsRead = 0;

  while (geojson_sbp_read_frame(file, frame) && count < GRAPH_MAX_POINTS) {
    if (gpsIndex >= startGpsIdx && gpsIndex <= endGpsIdx) {
      sumCms += frame.Sog;
      samplesInSecond++;

      if (samplesInSecond >= sampleRate) {
        if (((secIndex - startSecIdx) % secStep) == 0) {
          const float avgMmps = (static_cast<float>(sumCms) * 10.0f) / sampleRate;
          out[count++] = avgMmps * MMPS_TO_KNOTS;
        }
        sumCms = 0;
        samplesInSecond = 0;
        secIndex++;
        secondsRead++;
      }
    }
    if (gpsIndex > endGpsIdx) break;
    gpsIndex++;
  }

  file.close();
  graph1hMinutes = secondsRead / 60.0f;
  return count;
}

int readSecondSpeedGraphForGpsRange(const char* sbpPath, float* out, int startGpsIdx, int endGpsIdx)
{
  if (startGpsIdx < 1 || endGpsIdx < startGpsIdx) return 0;

  const int sampleRate = systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 1;
  const int startSecIdx = (startGpsIdx - 1) / sampleRate;
  const int endSecIdx = (endGpsIdx - 1) / sampleRate;
  return readSecondSpeedGraph(sbpPath, out, startSecIdx, endSecIdx);
}

int readSessionSpeedGraph(const char* sbpPath)
{
  graphSessionMinutes = 0.0f;
  File file;
  if (!geojson_sbp_open(file, sbpPath)) return 0;

  const int sampleRate = systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 1;
  int count = 0;
  int strideSamples = sampleRate * 5;
  if (strideSamples < 1) strideSamples = 1;
  int nextSample = 1;
  int gpsIndex = 1;

  GeoJsonSbpFrame frame;
  while (geojson_sbp_read_frame(file, frame)) {
    if (gpsIndex >= nextSample) {
      appendCompactGraphPoint(graphDistance, count, strideSamples, geojson_sbp_frame_knots(frame));
      nextSample = gpsIndex + strideSamples;
    }

    gpsIndex++;
  }

  file.close();
  graphSessionMinutes = (gpsIndex - 1) / static_cast<float>(sampleRate) / 60.0f;
  return count;
}

void attachGraphSeries(const char* sbpPath, const SessionStatsSnapshot& snapshot)
{
  const int s2Count = readGpsSpeedGraph(
    sbpPath,
    graph2s,
    snapshot.max2s.startSbp,
    snapshot.max2s.endSbp
  );

  int s10SeriesCount = 0;
  for (int i = 0; i < 5; i++) {
    graph10sCount[i] = 0;
  }

  for (int i = 0; i < 5; i++) {
    if (!hasWindow(snapshot.tenSecond[i])) continue;

    graph10sCount[i] = readGpsSpeedGraph(
      sbpPath,
      graph10s[i],
      snapshot.tenSecond[i].startSbp,
      snapshot.tenSecond[i].endSbp
    );
    s10SeriesCount = i + 1;
  }

  const int alphaCount = readGpsSpeedGraph(
    sbpPath,
    graphAlpha,
    snapshot.alpha.startSbp,
    snapshot.alpha.endSbp
  );
  const int nmCount = readGpsSpeedGraph(
    sbpPath,
    graphNm,
    snapshot.nauticalMile.startSbp,
    snapshot.nauticalMile.endSbp
  );
  int h1Count = readSecondSpeedGraphForGpsRange(
    sbpPath,
    graph1h,
    snapshot.oneHour.startSbp,
    snapshot.oneHour.endSbp
  );
  if (h1Count == 0) {
    const int sampleRate = systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 1;
    const int totalSeconds = geojson_sbp_count_frames(sbpPath) / sampleRate;
    if (totalSeconds > 0) {
      h1Count = readSecondSpeedGraph(sbpPath, graph1h, 0, totalSeconds - 1);
    }
  }
  const int distanceCount = readSessionSpeedGraph(sbpPath);

  GeoJSONGraphs graphs {
    .s2 = { graph2s, s2Count, 2.0f, "s" },
    .s10 = {
      { graph10s[0], graph10sCount[0], 10.0f, "s" },
      { graph10s[1], graph10sCount[1], 10.0f, "s" },
      { graph10s[2], graph10sCount[2], 10.0f, "s" },
      { graph10s[3], graph10sCount[3], 10.0f, "s" },
      { graph10s[4], graph10sCount[4], 10.0f, "s" }
    },
    .s10Count = s10SeriesCount,
    .alpha = { graphAlpha, alphaCount, 500.0f, "m" },
    .nm = { graphNm, nmCount, 1852.0f, "m" },
    .h1 = { graph1h, h1Count, graph1hMinutes, "min" },
    .distance = { graphDistance, distanceCount, graphSessionMinutes, "min" }
  };

  geojson_set_graphs(graphs);
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
  if (samples / step > GRAPH_MAX_POINTS) {
    step = (samples + GRAPH_MAX_POINTS - 1) / GRAPH_MAX_POINTS;
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
  int secStep = seconds > GRAPH_MAX_POINTS ? (seconds + GRAPH_MAX_POINTS - 1) / GRAPH_MAX_POINTS : 1;

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
  attachGraphSeries(sbpPath, snapshot);
  geojson_end_feature();
  addDerivedFeatures(sbpPath, snapshot);
  geojson_end();
  return true;
}
