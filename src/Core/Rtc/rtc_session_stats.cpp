#include "Core/Rtc/rtc_session_stats.h"

#include "Core/build_config.h"
#include "Core/log.h"
#include "Core/system_info.h"

#include "GPS/Data/gps_data.h"
#include "GPS/Data/gps_runtime_instances.h"
#include "GPS/gps_config.h"
#include "GPS/Metrics/gps_alpha_speed.h"
#include "GPS/Metrics/gps_distance_speed.h"
#include "GPS/Metrics/gps_result_sort.h"
#include "GPS/Metrics/gps_run_detector.h"
#include "GPS/Metrics/gps_time_speed.h"
#include "Logging/sbp_writer.h"

#include <math.h>

RTC_DATA_ATTR float RTC_distance = 0.0f;

RTC_DATA_ATTR float RTC_avg_10s_knots = 0.0f;
RTC_DATA_ATTR float RTC_max_2s_knots = 0.0f;
RTC_DATA_ATTR float RTC_alp_knots = 0.0f;
RTC_DATA_ATTR float RTC_1h_knots = 0.0f;
RTC_DATA_ATTR float RTC_mile_knots = 0.0f;

RTC_DATA_ATTR float RTC_R1_10s = 0.0f;
RTC_DATA_ATTR float RTC_R2_10s = 0.0f;
RTC_DATA_ATTR float RTC_R3_10s = 0.0f;
RTC_DATA_ATTR float RTC_R4_10s = 0.0f;
RTC_DATA_ATTR float RTC_R5_10s = 0.0f;

namespace {
int last_printed_run = -1;

int sampleRate()
{
  return systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 1;
}

void printSbpRange(const char* label, int first, int last)
{
  if (first < 0 || last < first) {
    Serial.printf("%-15s : n/a\n", label);
    return;
  }

  Serial.printf("%-15s : %d -> %d\n", label, first, last);
}

void printSnapshotHeader()
{
  Serial.println("\n================ GPS STATS SUMMARY ================");
}

void printSnapshotFooter()
{
  Serial.println("================ END GPS STATS ====================\n");
}

void printRunStartDebug()
{
  const int currentRun = gps_run_current();
  if (currentRun == last_printed_run || !gps_run_started()) return;

  last_printed_run = currentRun;
  Serial.printf("[RUN ] #%d started at GPS sample %d\n", currentRun, index_GPS);
}

void snapshotBest2s()
{
  RTC_max_2s_knots = speed_2s.s_max_speed * MMPS_TO_KNOTS;
  Serial.printf("2s max          : %.3f kn\n", RTC_max_2s_knots);
}

void clearTopRun10sSlots()
{
  RTC_R1_10s = 0.0f;
  RTC_R2_10s = 0.0f;
  RTC_R3_10s = 0.0f;
  RTC_R4_10s = 0.0f;
  RTC_R5_10s = 0.0f;
}

void storeTopRun10sSlot(int rank, float valueKnots)
{
  const int run = rank < win_10s_top5_count ? win_10s_top5_run[rank] : -1;

  switch (rank) {
    case 0:
      RTC_R1_10s = valueKnots;
      Serial.printf("  #1            : %.3f kn run=%d\n", valueKnots, run);
      break;
    case 1:
      RTC_R2_10s = valueKnots;
      Serial.printf("  #2            : %.3f kn run=%d\n", valueKnots, run);
      break;
    case 2:
      RTC_R3_10s = valueKnots;
      Serial.printf("  #3            : %.3f kn run=%d\n", valueKnots, run);
      break;
    case 3:
      RTC_R4_10s = valueKnots;
      Serial.printf("  #4            : %.3f kn run=%d\n", valueKnots, run);
      break;
    case 4:
      RTC_R5_10s = valueKnots;
      Serial.printf("  #5            : %.3f kn run=%d\n", valueKnots, run);
      break;
    default:
      break;
  }
}

void snapshotTopRun10s()
{
  gps_time_speed_rebuild_10s_top5_per_run();

  clearTopRun10sSlots();

  double topSumKnots = 0.0;

  Serial.println("10s best (top 5 runs):");

  for (int rank = 0; rank < 5; rank++) {
    const float valueKnots =
        rank < win_10s_top5_count ? win_10s_top5_speed[rank] * MMPS_TO_KNOTS : 0.0f;

    topSumKnots += valueKnots;
    storeTopRun10sSlot(rank, valueKnots);
  }

  RTC_avg_10s_knots = topSumKnots / 5.0f;
  Serial.printf("Run count       : %d\n", speed_10s.run_count);
  Serial.printf("10s avg (best 5): %.3f kn\n", RTC_avg_10s_knots);
}

void printRun10sSummary()
{
  const int rate = sampleRate();
  const int maxRun = speed_10s.run_count < MAX_10S_RUNS
                       ? speed_10s.run_count
                       : MAX_10S_RUNS - 1;

  Serial.println();
  Serial.println("10s per-run bests:");
  for (int run = 1; run <= maxRun; run++) {
    if (speed_10s.best_10s_per_run[run] <= 0.0) continue;

    const int first = win_10s_sbp_start_run[run];
    const int last = first >= 1 ? first + (10 * rate) - 1 : -1;
    Serial.printf("  R%-2d %.3f kn %d -> %d\n",
                  run,
                  speed_10s.best_10s_per_run[run] * MMPS_TO_KNOTS,
                  first,
                  last);
  }
}

void printRunDetectorSummary()
{
  Serial.println();
  Serial.println("Run detector:");
  Serial.printf("  armed          : %d last=%d\n",
                gps_run_armed_count(),
                gps_run_last_armed_index());
  Serial.printf("  jibes          : %d last=%d\n",
                gps_run_jibe_count(),
                gps_run_last_jibe_index());
  Serial.printf("  standstill     : %d\n", gps_run_standstill_restart_count());
  Serial.printf("  current run    : %d\n", gps_run_current());
}

void printStatSbpWindows()
{
  const int rate = sampleRate();

  Serial.println();
  Serial.println("SBP windows (first -> last point):");

  printSbpRange("2s", win_2s_sbp_start, win_2s_sbp_start >= 1 ? win_2s_sbp_start + (2 * rate) - 1 : -1);

  for (int i = 0; i < 5; i++) {
    char label[16];
    snprintf(label, sizeof(label), "10s #%d R%d", i + 1, win_10s_top5_run[i]);
    printSbpRange(
      label,
      win_10s_top5_sbp_start[i],
      win_10s_top5_sbp_start[i] >= 1 ? win_10s_top5_sbp_start[i] + (10 * rate) - 1 : -1
    );
  }

  printSbpRange("NM", win_nm_start, win_nm_end);
  printSbpRange("Alpha", alpha_sbp_start, alpha_sbp_end);
  if (alpha_sbp_start >= 0 && alpha_sbp_end >= alpha_sbp_start) {
    Serial.printf("%-15s : %.3f kn, %dm path, %.1fm closure\n",
                  "Alpha detail",
                  alpha_best_speed_mmps * MMPS_TO_KNOTS,
                  alpha_best_distance_m,
                  alpha_best_closure_m);
  }

  const int h1StartGps = win_1h_start_sec >= 0 ? (win_1h_start_sec * rate) + 1 : -1;
  const int h1EndGps = win_1h_end_sec >= 0 ? (win_1h_end_sec + 1) * rate : -1;
  printSbpRange("1 hour", h1StartGps, h1EndGps);

  const int sbpFrames = sbp_writer_frame_count();
  printSbpRange("Distance", sbpFrames > 0 ? 1 : -1, sbpFrames);
}

double bestDistanceSpeedMmps(const GPS_distance_speed& window)
{
  double best = window.m_max_speed;

  for (int i = 0; i < 10; i++) {
    if (window.avg_speed[i] > best) best = window.avg_speed[i];
  }

  return best;
}

void snapshotSpecialSpeeds()
{
  RTC_mile_knots = bestDistanceSpeedMmps(speed_nm) * MMPS_TO_KNOTS;

  // The map/export alpha geometry is captured as a session-best candidate while
  // riding. Use it as a fallback so the final number matches the green alpha
  // segment even if the ranked alpha array has not retained that candidate.
  const float rankedAlphaMmps = alpha_500m.avg_speed[9];
  const float bestAlphaMmps =
      rankedAlphaMmps > alpha_best_speed_mmps ? rankedAlphaMmps : alpha_best_speed_mmps;
  RTC_alp_knots = bestAlphaMmps * MMPS_TO_KNOTS;

  const float padded1hMmps = total_distance / 3600.0f;
  RTC_1h_knots = speed_1h.s_max_speed > 0.0
                   ? speed_1h.s_max_speed * MMPS_TO_KNOTS
                   : padded1hMmps * MMPS_TO_KNOTS;

  Serial.printf("NM (1852m)      : %.3f kn\n", RTC_mile_knots);
  Serial.printf("Alpha 500       : %.3f kn\n", RTC_alp_knots);
  Serial.printf("1 hour          : %.3f kn\n", RTC_1h_knots);
}

void printAlphaRankTable(const char* title, const Alfa_speed& alpha)
{
  Serial.println(title);

  for (int rank = 0; rank < 5; rank++) {
    const int slot = 9 - rank;
    const float speedKnots = alpha.avg_speed[slot] * MMPS_TO_KNOTS;
    const int start = alpha.ResultSbpStart(slot);
    const int end = alpha.ResultSbpEnd(slot);

    if (speedKnots <= 0.0f || start < 0 || end < start) {
      Serial.printf("  #%d            : n/a\n", rank + 1);
      continue;
    }

    const int pathM = alpha.alfa_distance[slot] / 1000;
    const float closureM = sqrt((double)alpha.real_distance[slot]);
    Serial.printf("  #%d            : %.3f kn %d -> %d dist=%dm closure=%.1fm\n",
                  rank + 1,
                  speedKnots,
                  start,
                  end,
                  pathM,
                  closureM);
  }
}

void printAlphaComparisonTables()
{
  Serial.println();
  printAlphaRankTable("Alpha 50m top 5:", alpha_500m);
#if STATS_ONLY_SERIAL
  printAlphaRankTable("Alpha 60m top 5:", alpha_500m_60);
  printAlphaRankTable("Alpha 70m top 5:", alpha_500m_70);
#endif
}

void snapshotDistance()
{
  RTC_distance = total_distance * 0.000001f;   // mm -> km
  Serial.printf("Distance        : %.3f km\n", RTC_distance);
}

void printFinalScreenValues()
{
  Serial.println();
  Serial.println("Final screen values:");
  Serial.printf("  02: %.3f kn\n", RTC_max_2s_knots);
  Serial.printf("  10: %.3f kn\n", RTC_avg_10s_knots);
  Serial.printf("  1H: %.3f kn\n", RTC_1h_knots);
  Serial.printf("  AL: %.3f kn\n", RTC_alp_knots);
  Serial.printf("  NM: %.3f kn\n", RTC_mile_knots);
  Serial.printf("  DI: %.3f km\n", RTC_distance);
}
} // namespace

void rtc_snapshot_stats()
{
  printSnapshotHeader();
  snapshotBest2s();
  snapshotTopRun10s();
  snapshotSpecialSpeeds();
  printAlphaComparisonTables();
  snapshotDistance();
  printStatSbpWindows();
  printSnapshotFooter();
}
