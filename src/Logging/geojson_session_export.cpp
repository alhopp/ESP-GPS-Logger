#include "Logging/geojson_session_export.h"

#include <Arduino.h>

#include "Core/log.h"
#include "Core/Rtc/rtc_session_stats.h"
#include "Core/system_info.h"
#include "GPS/Data/gps_data.h"
#include "GPS/gps_config.h"
#include "GPS/Metrics/gps_alpha_speed.h"
#include "GPS/Metrics/gps_distance_speed.h"
#include "GPS/Metrics/gps_time_speed.h"
#include "Logging/geojson_writer.h"
#include "Storage/storage_manager.h"

namespace {
constexpr int SBP_HEADER_SIZE = 64;
constexpr int GRAPH_MAX_POINTS = 240;

struct SBPFrame {
  uint8_t  HDOP;
  uint8_t  SVIDCnt;
  uint16_t UtcSec;
  uint32_t date_time_UTC_packed;
  uint32_t SVIDList;
  int32_t  Lat;
  int32_t  Lon;
  int32_t  AltCM;
  uint16_t Sog;
  uint16_t Cog;
  int16_t  ClmbRte;
  uint8_t  sdop;
  uint8_t  vsdop;
} __attribute__((packed));

float graph2s[GRAPH_MAX_POINTS];
float graph10s[5][GRAPH_MAX_POINTS];
int graph10sCount[5];
float graphAlpha[GRAPH_MAX_POINTS];
float graphNm[GRAPH_MAX_POINTS];
float graph1h[GRAPH_MAX_POINTS];
float graphDistance[GRAPH_MAX_POINTS];
float graph1hMinutes = 0.0f;
float graphSessionMinutes = 0.0f;

double frameLat(const SBPFrame& frame)
{
  return frame.Lat * 0.0000001;
}

double frameLon(const SBPFrame& frame)
{
  return frame.Lon * 0.0000001;
}

float frameKnots(const SBPFrame& frame)
{
  return static_cast<float>(frame.Sog) * 10.0f * MMPS_TO_KNOTS;
}

bool openSbp(File& file, const char* sbpPath)
{
  fs::FS& storage = storage_sd_fs();
  file = storage.open(sbpPath, FILE_READ);
  if (!file) return false;
  if (file.size() <= SBP_HEADER_SIZE) {
    file.close();
    return false;
  }
  file.seek(SBP_HEADER_SIZE);
  return true;
}

bool readFrame(File& file, SBPFrame& frame)
{
  return file.read(reinterpret_cast<uint8_t*>(&frame), sizeof(frame)) == sizeof(frame);
}

int countSbpFrames(const char* sbpPath)
{
  fs::FS& storage = storage_sd_fs();
  File file = storage.open(sbpPath, FILE_READ);
  if (!file) return 0;

  const size_t size = file.size();
  file.close();

  if (size <= SBP_HEADER_SIZE) return 0;
  return (size - SBP_HEADER_SIZE) / sizeof(SBPFrame);
}

void attachSessionStats()
{
  const float alphaKnotsFromBest = alpha_best_speed_mmps * MMPS_TO_KNOTS;

  GeoJSONStats s {
    .nm = RTC_mile_knots,
    .alpha = RTC_alp_knots > alphaKnotsFromBest ? RTC_alp_knots : alphaKnotsFromBest,
    .alphaDistance = static_cast<float>(alpha_best_distance_m),
    .alphaClosure = alpha_best_closure_m,
    .h1 = RTC_1h_knots,
    .max = RTC_max_2s_knots,
    .avg10 = RTC_avg_10s_knots,
    .r10 = {
      RTC_R1_10s,
      RTC_R2_10s,
      RTC_R3_10s,
      RTC_R4_10s,
      RTC_R5_10s
    },
    .distance = RTC_distance
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
  if (!openSbp(file, sbpPath)) return 0;

  SBPFrame frame;
  int gpsIndex = 1;
  int count = 0;
  while (readFrame(file, frame) && count < GRAPH_MAX_POINTS) {
    if (gpsIndex >= startGpsIdx && gpsIndex <= endGpsIdx &&
        ((gpsIndex - startGpsIdx) % step) == 0) {
      out[count++] = frameKnots(frame);
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
  if (!openSbp(file, sbpPath)) return 0;

  SBPFrame frame;
  int gpsIndex = 1;
  int secIndex = startSecIdx;
  int count = 0;
  uint32_t sumCms = 0;
  int samplesInSecond = 0;
  int secondsRead = 0;

  while (readFrame(file, frame) && count < GRAPH_MAX_POINTS) {
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

int readSessionSpeedGraph(const char* sbpPath)
{
  graphSessionMinutes = 0.0f;
  File file;
  if (!openSbp(file, sbpPath)) return 0;

  const int sampleRate = systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 1;
  int count = 0;
  int strideSamples = sampleRate * 5;
  if (strideSamples < 1) strideSamples = 1;
  int nextSample = 1;
  int gpsIndex = 1;

  SBPFrame frame;
  while (readFrame(file, frame)) {
    if (gpsIndex >= nextSample) {
      appendCompactGraphPoint(graphDistance, count, strideSamples, frameKnots(frame));
      nextSample = gpsIndex + strideSamples;
    }

    gpsIndex++;
  }

  file.close();
  graphSessionMinutes = (gpsIndex - 1) / static_cast<float>(sampleRate) / 60.0f;
  return count;
}

void attachGraphSeries(const char* sbpPath)
{
  const int s2Count = readGpsSpeedGraph(
    sbpPath,
    graph2s,
    win_2s_sbp_start,
    win_2s_sbp_start >= 1 ? win_2s_sbp_start + (2 * systemInfo.sample_rate) - 1 : -1
  );

  const int s10SeriesCount = win_10s_top5_count > 5 ? 5 : win_10s_top5_count;
  for (int i = 0; i < 5; i++) {
    graph10sCount[i] = 0;
  }
  for (int i = 0; i < s10SeriesCount; i++) {
    graph10sCount[i] = readGpsSpeedGraph(
      sbpPath,
      graph10s[i],
      win_10s_top5_sbp_start[i],
      win_10s_top5_sbp_start[i] >= 1 ? win_10s_top5_sbp_start[i] + (10 * systemInfo.sample_rate) - 1 : -1
    );
  }

  const int alphaCount = readGpsSpeedGraph(sbpPath, graphAlpha, alpha_sbp_start, alpha_sbp_end);
  const int nmCount = readGpsSpeedGraph(sbpPath, graphNm, win_nm_start, win_nm_end);
  int h1Count = readSecondSpeedGraph(sbpPath, graph1h, win_1h_start_sec, win_1h_end_sec);
  if (h1Count == 0) {
    const int sampleRate = systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 1;
    const int totalSeconds = countSbpFrames(sbpPath) / sampleRate;
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
  if (!openSbp(file, sbpPath)) return;

  SBPFrame frame;
  int gpsIndex = 1;
  while (readFrame(file, frame)) {
    if (gpsIndex >= startGpsIdx && gpsIndex <= endGpsIdx &&
        ((gpsIndex - startGpsIdx) % step) == 0) {
      geojson_add_point(frameLat(frame), frameLon(frame));
    }
    if (gpsIndex > endGpsIdx) break;
    gpsIndex++;
  }

  file.close();
}

void addWindowFeature(const char* sbpPath, const char* mode, int startGpsIdx, int seconds)
{
  if (startGpsIdx < 0) return;

  const int sampleRate = systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 1;
  geojson_begin_feature(mode);
  addSbpRangePoints(sbpPath, startGpsIdx, startGpsIdx + (seconds * sampleRate) - 1, sampleRate);
  geojson_end_feature();
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

void addOneHourFeature(const char* sbpPath)
{
  if (win_1h_start_sec < 0 || win_1h_end_sec < win_1h_start_sec) return;

  const int sampleRate = systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 1;
  const int startGpsIdx = (win_1h_start_sec * sampleRate) + 1;
  const int endGpsIdx = (win_1h_end_sec + 1) * sampleRate;
  const int seconds = win_1h_end_sec - win_1h_start_sec + 1;
  int secStep = seconds > GRAPH_MAX_POINTS ? (seconds + GRAPH_MAX_POINTS - 1) / GRAPH_MAX_POINTS : 1;

  geojson_begin_feature("1h");
  addSbpRangePoints(sbpPath, startGpsIdx, endGpsIdx, secStep * sampleRate);
  geojson_end_feature();
}

void addDerivedFeatures(const char* sbpPath)
{
  addWindowFeature(sbpPath, "2s", win_2s_sbp_start, 2);

  for (int i = 0; i < win_10s_top5_count; i++) {
    addWindowFeature(sbpPath, "10s", win_10s_top5_sbp_start[i], 10);
  }

  addRangeFeature(sbpPath, "alpha", alpha_sbp_start, alpha_sbp_end);
  addRangeFeature(sbpPath, "nm", win_nm_start, win_nm_end);
  addOneHourFeature(sbpPath);
}

bool addBaseTrackFromSbp(const char* sbpPath)
{
  File file;
  if (!openSbp(file, sbpPath)) return false;

  SBPFrame frame;
  while (readFrame(file, frame)) {
    geojson_add_track_point(frameLat(frame), frameLon(frame));
  }

  file.close();
  return true;
}
}

bool geojson_session_export_finalize(const char* sbpPath, const char* geojsonPath)
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

  attachSessionStats();
  attachGraphSeries(sbpPath);
  geojson_end_feature();
  addDerivedFeatures(sbpPath);
  geojson_end();
  return true;
}
