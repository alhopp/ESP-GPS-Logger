#include "Core/Rtc/rtc_session_stats.h"

#include "Core/log.h"

#include "GPS/Data/gps_data.h"
#include "GPS/gps_config.h"
#include "GPS/Metrics/gps_alpha_speed.h"
#include "GPS/Metrics/gps_distance_speed.h"
#include "GPS/Metrics/gps_result_sort.h"
#include "GPS/Metrics/gps_run_detector.h"
#include "GPS/Metrics/gps_time_speed.h"

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

void printSnapshotHeader()
{
  Serial.println("\n================ RTC SNAPSHOT STATS ================");
}

void printSnapshotFooter()
{
  Serial.println("====================================================\n");
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
  RTC_max_2s_knots = S2.s_max_speed * MMPS_TO_KNOTS;
  Serial.printf("2s max          : %.3f kn\n", RTC_max_2s_knots);
}

int collectSortedRun10s(double results[], int maxResults)
{
  int count = 0;

  for (int run = 1; run <= S10.run_count && run < maxResults; run++) {
    if (S10.best_10s_per_run[run] > 0) {
      results[count++] = S10.best_10s_per_run[run];
    }
  }

  if (count > 1) {
    sort_display(results, count);
  }

  return count;
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
  switch (rank) {
    case 0:
      RTC_R1_10s = valueKnots;
      Serial.printf("  #1            : %.3f kn\n", valueKnots);
      break;
    case 1:
      RTC_R2_10s = valueKnots;
      Serial.printf("  #2            : %.3f kn\n", valueKnots);
      break;
    case 2:
      RTC_R3_10s = valueKnots;
      Serial.printf("  #3            : %.3f kn\n", valueKnots);
      break;
    case 3:
      RTC_R4_10s = valueKnots;
      Serial.printf("  #4            : %.3f kn\n", valueKnots);
      break;
    case 4:
      RTC_R5_10s = valueKnots;
      Serial.printf("  #5            : %.3f kn\n", valueKnots);
      break;
    default:
      break;
  }
}

void snapshotTopRun10s()
{
  double runResults[32];
  const int resultCount = collectSortedRun10s(runResults, 32);

  clearTopRun10sSlots();

  const int topCount = (resultCount >= 5) ? 5 : resultCount;
  double topSumKnots = 0.0;

  Serial.println("10s best (top 5 runs):");

  for (int rank = 0; rank < topCount; rank++) {
    const float valueKnots = runResults[resultCount - 1 - rank] * MMPS_TO_KNOTS;
    topSumKnots += valueKnots;
    storeTopRun10sSlot(rank, valueKnots);
  }

  RTC_avg_10s_knots = topSumKnots / 5.0f;
  Serial.printf("10s avg (best %d): %.3f kn\n", topCount, RTC_avg_10s_knots);
}

void snapshotSpecialSpeeds()
{
  RTC_mile_knots = M1852.avg_speed[9] * MMPS_TO_KNOTS;
  RTC_alp_knots = A500.avg_speed[9] * MMPS_TO_KNOTS;
  RTC_1h_knots = S3600.s_max_speed * MMPS_TO_KNOTS;

  Serial.printf("NM (1852m)      : %.3f kn\n", RTC_mile_knots);
  Serial.printf("Alpha 500       : %.3f kn\n", RTC_alp_knots);
  Serial.printf("1 hour          : %.3f kn\n", RTC_1h_knots);
}

void snapshotDistance()
{
  RTC_distance = total_distance * 0.000001f;   // mm -> km
  Serial.printf("Distance        : %.3f km\n", RTC_distance);
}
} // namespace

void rtc_snapshot_stats()
{
  printSnapshotHeader();
  printRunStartDebug();
  snapshotBest2s();
  snapshotTopRun10s();
  snapshotSpecialSpeeds();
  snapshotDistance();
  printSnapshotFooter();
}
