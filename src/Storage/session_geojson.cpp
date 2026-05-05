#include "Storage/session_geojson.h"

#include "Core/Definitions.h"
#include "Core/rtc_state.h"
#include "Core/system_info.h"
#include "GPS/GPS_data.h"
#include "GPS/gps_alpha.h"
#include "GPS/gps_speed.h"
#include "GPS/gps_time.h"
#include "Storage/geojson.h"

namespace {
bool validGpsIndex(int i)
{
  return i >= 0 && i < BUFFER_SIZE;
}

void addRingWindow1Hz(int startGpsIdx, int seconds)
{
  for (int s = 0; s < seconds; s++) {
    int idx = startGpsIdx + s * systemInfo.sample_rate;
    idx %= BUFFER_SIZE;
    if (idx < 0) idx += BUFFER_SIZE;

    if (validGpsIndex(idx)) {
      geojson_add_point(_lat[idx], _long[idx]);
    }
  }
}

void addRingRange1Hz(int start, int end)
{
  int idx = start;
  int step = 0;

  for (int guard = 0; guard < BUFFER_SIZE; guard++) {
    if (step % systemInfo.sample_rate == 0 && validGpsIndex(idx)) {
      geojson_add_point(_lat[idx], _long[idx]);
    }

    if (idx == end) break;

    idx++;
    if (idx >= BUFFER_SIZE) idx = 0;
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

void addDerivedFeatures()
{
  if (win_2s_start >= 0) {
    geojson_begin_feature("2s");
    addRingWindow1Hz(win_2s_start, 2);
    geojson_end_feature();
  }

  for (int i = 0; i < win_10s_top5_count; i++) {
    int s = win_10s_top5_start[i];
    if (s < 0) continue;

    geojson_begin_feature("10s");
    addRingWindow1Hz(s, 10);
    geojson_end_feature();
  }

  if (alpha_start >= 0 && alpha_end >= 0) {
    geojson_begin_feature("alpha");
    addRingRange1Hz(alpha_start, alpha_end);
    geojson_end_feature();
  }

  if (win_nm_start >= 0 && win_nm_end >= 0) {
    geojson_begin_feature("nm");
    addRingRange1Hz(win_nm_start, win_nm_end);
    geojson_end_feature();
  }

  if (win_1h_start_sec >= 0 && win_1h_end_sec > win_1h_start_sec) {
    geojson_begin_feature("1h");
    for (int s = win_1h_start_sec; s <= win_1h_end_sec; s++) {
      int idx = sec_to_gps_index[s];
      if (validGpsIndex(idx)) {
        geojson_add_point(_lat[idx], _long[idx]);
      }
    }
    geojson_end_feature();
  }
}
}

void session_geojson_finalize()
{
  attachSessionStats();
  geojson_end_feature();
  addDerivedFeatures();
  geojson_end();
}

