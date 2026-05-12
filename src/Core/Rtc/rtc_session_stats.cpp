#include "Core/Rtc/rtc_session_stats.h"

#include "Core/build_config.h"

#include "GPS/Data/gps_runtime_instances.h"
#include "GPS/gps_config.h"
#include "GPS/Metrics/gps_alpha_speed.h"
#include "Session/session_stats_snapshot.h"

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

void applySnapshotToRtc(const SessionStatsSnapshot& snapshot)
{
  RTC_max_2s_knots = snapshot.max2s.speedKnots;
  RTC_avg_10s_knots = snapshot.tenSecondAverageKnots;
  RTC_mile_knots = snapshot.nauticalMile.speedKnots;
  RTC_alp_knots = snapshot.alpha.speedKnots;
  RTC_1h_knots = snapshot.oneHour.speedKnots;
  RTC_distance = snapshot.distanceKm;

  RTC_R1_10s = snapshot.tenSecond[0].speedKnots;
  RTC_R2_10s = snapshot.tenSecond[1].speedKnots;
  RTC_R3_10s = snapshot.tenSecond[2].speedKnots;
  RTC_R4_10s = snapshot.tenSecond[3].speedKnots;
  RTC_R5_10s = snapshot.tenSecond[4].speedKnots;
}

void printSnapshotStats(const SessionStatsSnapshot& snapshot)
{
  Serial.printf("2s max          : %.3f kn\n", snapshot.max2s.speedKnots);

  Serial.println("10s best (top 5 runs):");
  for (int rank = 0; rank < 5; rank++) {
    Serial.printf("  #%d            : %.3f kn run=%d\n",
                  rank + 1,
                  snapshot.tenSecond[rank].speedKnots,
                  snapshot.tenSecond[rank].run);
  }
  Serial.printf("Run count       : %d\n", snapshot.runCount);
  Serial.printf("10s avg (best 5): %.3f kn\n", snapshot.tenSecondAverageKnots);

  Serial.printf("NM (1852m)      : %.3f kn\n", snapshot.nauticalMile.speedKnots);
  Serial.printf("Alpha 500       : %.3f kn\n", snapshot.alpha.speedKnots);
  Serial.printf("1 hour          : %.3f kn\n", snapshot.oneHour.speedKnots);
}

void printSnapshotDistance(const SessionStatsSnapshot& snapshot)
{
  Serial.printf("Distance        : %.3f km\n", snapshot.distanceKm);
}

void printStatSbpWindows(const SessionStatsSnapshot& snapshot)
{
  Serial.println();
  Serial.println("SBP windows (first -> last point):");

  printSbpRange("2s", snapshot.max2s.startSbp, snapshot.max2s.endSbp);

  for (int i = 0; i < 5; i++) {
    char label[16];
    snprintf(label, sizeof(label), "10s #%d R%d", i + 1, snapshot.tenSecond[i].run);
    printSbpRange(label, snapshot.tenSecond[i].startSbp, snapshot.tenSecond[i].endSbp);
  }

  printSbpRange("NM", snapshot.nauticalMile.startSbp, snapshot.nauticalMile.endSbp);
  printSbpRange("Alpha", snapshot.alpha.startSbp, snapshot.alpha.endSbp);
  if (snapshot.alpha.startSbp >= 0 && snapshot.alpha.endSbp >= snapshot.alpha.startSbp) {
    Serial.printf("%-15s : %.3f kn, %dm path, %.1fm closure\n",
                  "Alpha detail",
                  snapshot.alpha.speedKnots,
                  snapshot.alpha.distanceM,
                  snapshot.alpha.closureM);
  }

  printSbpRange("1 hour", snapshot.oneHour.startSbp, snapshot.oneHour.endSbp);
  printSbpRange("Distance", snapshot.frameCount > 0 ? 1 : -1, snapshot.frameCount);
}
} // namespace

void rtc_snapshot_stats()
{
  const SessionStatsSnapshot snapshot = build_session_stats_snapshot();
  applySnapshotToRtc(snapshot);

  printSnapshotHeader();
  printSnapshotStats(snapshot);
  printAlphaComparisonTables();
  printSnapshotDistance(snapshot);
  printStatSbpWindows(snapshot);
  printSnapshotFooter();
}
