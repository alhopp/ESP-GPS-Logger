#include "Logging/geojson_session_export.h"

#include "Core/log.h"
#include "Core/rtc_state.h"
#include "Core/system_info.h"
#include "GPS/Data/gps_data.h"
#include "GPS/Metrics/gps_alpha_speed.h"
#include "GPS/Metrics/gps_distance_speed.h"
#include "GPS/Metrics/gps_time_speed.h"
#include "Logging/geojson_writer.h"

namespace {
bool validGpsIndex(int i)
{
  return i >= 0 && i < BUFFER_SIZE;
}

int wrapGpsIndex(int i)
{
  i %= BUFFER_SIZE;
  if (i < 0) i += BUFFER_SIZE;
  return i;
}

void addGpsPointIfValid(int idx)
{
  if (validGpsIndex(idx)) {
    geojson_add_point(_lat[idx], _long[idx]);
  }
}

void addRingWindow1Hz(int startGpsIdx, int seconds)
{
  for (int s = 0; s < seconds; s++) {
    int idx = wrapGpsIndex(startGpsIdx + s * systemInfo.sample_rate);
    addGpsPointIfValid(idx);
  }
}

void addRingRange1Hz(int start, int end)
{
  int idx = start;
  int step = 0;

  for (int guard = 0; guard < BUFFER_SIZE; guard++) {
    if (step % systemInfo.sample_rate == 0) {
      addGpsPointIfValid(idx);
    }

    if (idx == end) break;

    idx = wrapGpsIndex(idx + 1);
    step++;
  }
}

void attachSessionStats()
{
  GeoJSONStats s {
    .nm = RTC_mile_knots,
    .alpha = RTC_alp_knots,
    .h1 = RTC_1h_knots,
    .max = RTC_max_2s_knots,
    .avg10 = RTC_avg_10s_knots,
    .distance = RTC_distance
  };

  geojson_set_stats(s);
}

void addWindowFeature(const char* mode, int startGpsIdx, int seconds)
{
  geojson_begin_feature(mode);
  addRingWindow1Hz(startGpsIdx, seconds);
  geojson_end_feature();
}

void addRangeFeature(const char* mode, int startGpsIdx, int endGpsIdx)
{
  geojson_begin_feature(mode);
  addRingRange1Hz(startGpsIdx, endGpsIdx);
  geojson_end_feature();
}

void addDerivedFeatures()
{
  if (win_2s_start >= 0) {
    addWindowFeature("2s", win_2s_start, 2);
  }

  for (int i = 0; i < win_10s_top5_count; i++) {
    int s = win_10s_top5_start[i];
    if (s < 0) continue;

    addWindowFeature("10s", s, 10);
  }

  if (alpha_start >= 0 && alpha_end >= 0) {
    addRangeFeature("alpha", alpha_start, alpha_end);
  }

  if (win_nm_start >= 0 && win_nm_end >= 0) {
    addRangeFeature("nm", win_nm_start, win_nm_end);
  }

  if (win_1h_start_sec >= 0 && win_1h_end_sec > win_1h_start_sec) {
    geojson_begin_feature("1h");
    for (int s = win_1h_start_sec; s <= win_1h_end_sec; s++) {
      int idx = sec_to_gps_index[s];
      addGpsPointIfValid(idx);
    }
    geojson_end_feature();
  }
}
}

void geojson_session_export_finalize()
{
  attachSessionStats();
  geojson_end_feature();
  addDerivedFeatures();
  geojson_end();
}
