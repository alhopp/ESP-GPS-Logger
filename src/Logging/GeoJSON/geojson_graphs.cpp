// ============================================================================
// geojson_graphs.cpp
//
// Extracts downsampled speed graph series from a completed SBP session and
// attaches them to the active GeoJSON track feature.
// ============================================================================

#include "Logging/GeoJSON/geojson_graphs.h"

#include <Arduino.h>

#include "Core/system_info.h"
#include "GPS/gps_config.h"
#include "Logging/GeoJSON/geojson_export_limits.h"
#include "Logging/GeoJSON/geojson_sbp_reader.h"
#include "Logging/GeoJSON/geojson_session_windows.h"
#include "Logging/GeoJSON/geojson_writer.h"
#include "Session/session_stats_snapshot.h"

namespace {
struct GeoJsonGraphBuildState {
  float s2[GEOJSON_MAX_SERIES_POINTS];
  float s10[5][GEOJSON_MAX_SERIES_POINTS];
  int s10Count[5];
  float alpha[GEOJSON_MAX_SERIES_POINTS];
  float nm[GEOJSON_MAX_SERIES_POINTS];
  float h1[GEOJSON_MAX_SERIES_POINTS];
  float distance[GEOJSON_MAX_SERIES_POINTS];
  float h1Minutes = 0.0f;
  float sessionMinutes = 0.0f;
};

GeoJsonGraphBuildState graphState;

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
  if (count >= GEOJSON_MAX_SERIES_POINTS) {
    compressGraph(values, count, strideSamples);
  }
  values[count++] = value;
}

int readGpsSpeedGraph(const char* sbpPath, float* out, int startGpsIdx, int endGpsIdx)
{
  if (!out || startGpsIdx < 0 || endGpsIdx < startGpsIdx) return 0;

  const int samples = endGpsIdx - startGpsIdx + 1;
  const int step = samples > GEOJSON_MAX_SERIES_POINTS
      ? (samples + GEOJSON_MAX_SERIES_POINTS - 1) / GEOJSON_MAX_SERIES_POINTS
      : 1;

  File file;
  if (!geojson_sbp_open(file, sbpPath)) return 0;

  GeoJsonSbpFrame frame;
  int count = 0;
  for (int gpsIndex = startGpsIdx;
       gpsIndex <= endGpsIdx && count < GEOJSON_MAX_SERIES_POINTS;
       gpsIndex += step) {
    if (geojson_sbp_read_frame_at(file, gpsIndex, frame)) {
      out[count++] = geojson_sbp_frame_knots(frame);
    }
  }

  file.close();
  return count;
}

int readSecondSpeedGraph(const char* sbpPath, float* out, int startSecIdx, int endSecIdx)
{
  graphState.h1Minutes = 0.0f;
  if (!out || startSecIdx < 0 || endSecIdx < startSecIdx) return 0;

  const int sampleRate = systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 1;
  const int startGpsIdx = (startSecIdx * sampleRate) + 1;
  const int endGpsIdx = (endSecIdx + 1) * sampleRate;
  const int seconds = endSecIdx - startSecIdx + 1;
  const int secStep = seconds > GEOJSON_MAX_SERIES_POINTS
      ? (seconds + GEOJSON_MAX_SERIES_POINTS - 1) / GEOJSON_MAX_SERIES_POINTS
      : 1;

  File file;
  if (!geojson_sbp_open(file, sbpPath)) return 0;

  GeoJsonSbpFrame frame;
  int gpsIndex = 1;
  int secIndex = startSecIdx;
  int count = 0;
  uint32_t sumCms = 0;
  int samplesInSecond = 0;
  int secondsRead = 0;

  while (geojson_sbp_read_frame(file, frame) && count < GEOJSON_MAX_SERIES_POINTS) {
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
  graphState.h1Minutes = secondsRead / 60.0f;
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
  graphState.sessionMinutes = 0.0f;
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
      appendCompactGraphPoint(
        graphState.distance,
        count,
        strideSamples,
        geojson_sbp_frame_knots(frame)
      );
      nextSample = gpsIndex + strideSamples;
    }

    gpsIndex++;
  }

  file.close();
  graphState.sessionMinutes = (gpsIndex - 1) / static_cast<float>(sampleRate) / 60.0f;
  return count;
}
}

void geojson_attach_graph_series(const char* sbpPath, const SessionStatsSnapshot& snapshot)
{
  const int s2Count = readGpsSpeedGraph(
    sbpPath,
    graphState.s2,
    snapshot.max2s.startSbp,
    snapshot.max2s.endSbp
  );

  int s10SeriesCount = 0;
  for (int i = 0; i < 5; i++) {
    graphState.s10Count[i] = 0;
  }

  for (int i = 0; i < 5; i++) {
    if (!geojson_has_window(snapshot.tenSecond[i])) continue;

    graphState.s10Count[i] = readGpsSpeedGraph(
      sbpPath,
      graphState.s10[i],
      snapshot.tenSecond[i].startSbp,
      snapshot.tenSecond[i].endSbp
    );
    s10SeriesCount = i + 1;
  }

  const int alphaCount = readGpsSpeedGraph(
    sbpPath,
    graphState.alpha,
    snapshot.alpha.startSbp,
    snapshot.alpha.endSbp
  );
  const int nmCount = readGpsSpeedGraph(
    sbpPath,
    graphState.nm,
    snapshot.nauticalMile.startSbp,
    snapshot.nauticalMile.endSbp
  );
  int h1Count = readSecondSpeedGraphForGpsRange(
    sbpPath,
    graphState.h1,
    snapshot.oneHour.startSbp,
    snapshot.oneHour.endSbp
  );
  if (h1Count == 0) {
    const int sampleRate = systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 1;
    const int totalSeconds = geojson_sbp_count_frames(sbpPath) / sampleRate;
    if (totalSeconds > 0) {
      h1Count = readSecondSpeedGraph(sbpPath, graphState.h1, 0, totalSeconds - 1);
    }
  }
  const int distanceCount = readSessionSpeedGraph(sbpPath);

  GeoJSONGraphs graphs {
    .s2 = { graphState.s2, s2Count, 2.0f, "s" },
    .s10 = {
      { graphState.s10[0], graphState.s10Count[0], 10.0f, "s" },
      { graphState.s10[1], graphState.s10Count[1], 10.0f, "s" },
      { graphState.s10[2], graphState.s10Count[2], 10.0f, "s" },
      { graphState.s10[3], graphState.s10Count[3], 10.0f, "s" },
      { graphState.s10[4], graphState.s10Count[4], 10.0f, "s" }
    },
    .s10Count = s10SeriesCount,
    .alpha = { graphState.alpha, alphaCount, 500.0f, "m" },
    .nm = { graphState.nm, nmCount, 1852.0f, "m" },
    .h1 = { graphState.h1, h1Count, graphState.h1Minutes, "min" },
    .distance = { graphState.distance, distanceCount, graphState.sessionMinutes, "min" }
  };

  geojson_set_graphs(graphs);
}
