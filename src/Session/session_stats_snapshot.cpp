#include "Session/session_stats_snapshot.h"

#include "Core/system_info.h"
#include "GPS/Data/gps_data.h"
#include "GPS/Data/gps_runtime_instances.h"
#include "GPS/gps_config.h"
#include "GPS/Metrics/gps_alpha_speed.h"
#include "GPS/Metrics/gps_distance_speed.h"
#include "GPS/Metrics/gps_time_speed.h"
#include "Logging/sbp_writer.h"

namespace {
int sampleRate()
{
  return systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 1;
}

double bestDistanceSpeedMmps(const GPS_distance_speed& window)
{
  double best = window.m_max_speed;

  for (int i = 0; i < 10; i++) {
    if (window.avg_speed[i] > best) best = window.avg_speed[i];
  }

  return best;
}

int fixedWindowEnd(int startSbp, int seconds)
{
  if (startSbp < 1) return -1;
  return startSbp + (seconds * sampleRate()) - 1;
}

SessionWindow makeWindow(
  float speedKnots,
  int startSbp,
  int endSbp,
  int run = -1,
  uint32_t sumCms = 0
)
{
  SessionWindow window;
  window.speedKnots = speedKnots;
  window.startSbp = startSbp;
  window.endSbp = endSbp >= startSbp ? endSbp : -1;
  window.run = run;
  window.sumCms = sumCms;
  return window;
}
} // namespace

SessionStatsSnapshot build_session_stats_snapshot()
{
  gps_time_speed_rebuild_10s_top5_per_run();

  SessionStatsSnapshot snapshot;
  const int rate = sampleRate();

  snapshot.max2s = makeWindow(
    speed_2s.s_max_speed * MMPS_TO_KNOTS,
    win_2s_sbp_start,
    fixedWindowEnd(win_2s_sbp_start, 2),
    -1,
    win_2s_sum_cms
  );

  double tenSecondSumKnots = 0.0;
  for (int rank = 0; rank < 5; rank++) {
    const float speedKnots =
      rank < win_10s_top5_count ? win_10s_top5_speed[rank] * MMPS_TO_KNOTS : 0.0f;
    const int startSbp = rank < win_10s_top5_count ? win_10s_top5_sbp_start[rank] : -1;
    const int run = rank < win_10s_top5_count ? win_10s_top5_run[rank] : -1;

    snapshot.tenSecond[rank] = makeWindow(
      speedKnots,
      startSbp,
      fixedWindowEnd(startSbp, 10),
      run,
      rank < win_10s_top5_count ? win_10s_top5_sum_cms[rank] : 0
    );
    tenSecondSumKnots += speedKnots;
  }
  snapshot.tenSecondAverageKnots = tenSecondSumKnots / 5.0f;
  snapshot.runCount = speed_10s.run_count;

  snapshot.nauticalMile = makeWindow(
    bestDistanceSpeedMmps(speed_nm) * MMPS_TO_KNOTS,
    win_nm_start,
    win_nm_end
  );

  const float rankedAlphaMmps = alpha_500m.avg_speed[9];
  const float bestAlphaMmps =
    rankedAlphaMmps > alpha_best_speed_mmps ? rankedAlphaMmps : alpha_best_speed_mmps;
  snapshot.alpha.speedKnots = bestAlphaMmps * MMPS_TO_KNOTS;
  snapshot.alpha.startSbp = alpha_sbp_start;
  snapshot.alpha.endSbp = alpha_sbp_end;
  snapshot.alpha.distanceM = alpha_best_distance_m;
  snapshot.alpha.closureM = alpha_best_closure_m;

  const float padded1hMmps = total_distance / 3600.0f;
  const float hourKnots = speed_1h.s_max_speed > 0.0
    ? speed_1h.s_max_speed * MMPS_TO_KNOTS
    : padded1hMmps * MMPS_TO_KNOTS;
  const int hourStartSbp = win_1h_start_sec >= 0 ? (win_1h_start_sec * rate) + 1 : -1;
  const int hourEndSbp = win_1h_end_sec >= 0 ? (win_1h_end_sec + 1) * rate : -1;
  snapshot.oneHour = makeWindow(hourKnots, hourStartSbp, hourEndSbp);

  snapshot.distanceKm = total_distance * 0.000001f;
  snapshot.frameCount = sbp_writer_frame_count();

  return snapshot;
}
