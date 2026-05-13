// ============================================================================
// logging_sbp_debug.cpp
//
// Optional diagnostic reader for SBP session files. Prints selected SBP sample
// ranges used by the statistics pipeline without affecting session output.
// ============================================================================

#include "Logging/SBP/logging_sbp_debug.h"

#include <Arduino.h>
#include <FS.h>

#include "Core/build_config.h"
#include "GPS/gps_config.h"
#include "Session/session_stats_snapshot.h"
#include "Storage/storage_manager.h"

namespace {
constexpr int SBP_HEADER_SIZE = 64;

struct SBPDebugFrame {
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

float frameKnots(const SBPDebugFrame& frame)
{
  return static_cast<float>(frame.Sog) * 10.0f * MMPS_TO_KNOTS;
}

bool readSbpFrame(File& file, int sbpIndex, SBPDebugFrame& frame)
{
  if (sbpIndex < 1) return false;

  const size_t offset =
      SBP_HEADER_SIZE + static_cast<size_t>(sbpIndex - 1) * sizeof(SBPDebugFrame);
  if (offset + sizeof(SBPDebugFrame) > file.size()) return false;

  if (!file.seek(offset)) return false;
  return file.read(reinterpret_cast<uint8_t*>(&frame), sizeof(frame)) == sizeof(frame);
}

void printSbpSpeedRow(File& file, int first, int last)
{
  for (int index = first; index <= last; index++) {
    SBPDebugFrame frame;
    if (!readSbpFrame(file, index, frame)) {
      Serial.printf(" %d:n/a", index);
      continue;
    }

    Serial.printf(" %d:%.3f", index, frameKnots(frame));
  }
  Serial.println();
}

void printSbpSpeedRange(const char* label,
                        const char* sbpPath,
                        int first,
                        int last,
                        uint32_t calcSumCms)
{
  if (first < 1 || last < first) return;

  fs::FS& storage = storage_sd_fs();
  File file = storage.open(sbpPath, FILE_READ);
  if (!file) {
    Serial.printf("%s samples: file open failed\n", label);
    return;
  }

  Serial.printf("%s samples (idx:kn):\n", label);

  uint32_t sumCms = 0;
  int count = 0;
  constexpr int VALUES_PER_LINE = 5;
  for (int start = first; start <= last; start += VALUES_PER_LINE) {
    const int end = (start + VALUES_PER_LINE - 1) < last ? (start + VALUES_PER_LINE - 1) : last;
    printSbpSpeedRow(file, start, end);

    for (int index = start; index <= end; index++) {
      SBPDebugFrame frame;
      if (readSbpFrame(file, index, frame)) {
        sumCms += frame.Sog;
        count++;
      }
    }
  }

  if (count > 0) {
    const double avgMmps = (static_cast<double>(sumCms) * 10.0) / count;
    Serial.printf(
      "%s average: %.3f kn (%d samples, sum_cmps=%lu, calc_sum_cmps=%lu)\n",
      label,
      avgMmps * MMPS_TO_KNOTS,
      count,
      static_cast<unsigned long>(sumCms),
      static_cast<unsigned long>(calcSumCms)
    );
  }

  file.close();
}

void printSbpSpeedEdges(const char* label, const char* sbpPath, int first, int last, int edgeCount)
{
  if (first < 1 || last < first) return;

  fs::FS& storage = storage_sd_fs();
  File file = storage.open(sbpPath, FILE_READ);
  if (!file) {
    Serial.printf("%s samples: file open failed\n", label);
    return;
  }

  Serial.printf("%s samples first %d (idx:kn):\n", label, edgeCount);
  const int firstEnd = (first + edgeCount - 1) < last ? (first + edgeCount - 1) : last;
  printSbpSpeedRow(file, first, firstEnd);

  if (last > firstEnd) {
    const int lastStart = (last - edgeCount + 1) > first ? (last - edgeCount + 1) : first;
    Serial.printf("%s samples last %d (idx:kn):\n", label, edgeCount);
    printSbpSpeedRow(file, lastStart, last);
  }

  file.close();
}
}

void logging_sbp_debug_print_samples(const char* sbpPath, const SessionStatsSnapshot& snapshot)
{
#if LOG_ENABLED && SBP_STAT_SAMPLE_DEBUG
  if (!sbpPath || !sbpPath[0]) return;

  Serial.println();
  Serial.println("================ SBP STAT SAMPLE VALUES ================");

  printSbpSpeedRange(
    "2s",
    sbpPath,
    snapshot.max2s.startSbp,
    snapshot.max2s.endSbp,
    snapshot.max2s.sumCms
  );

  for (int i = 0; i < 5; i++) {
    char label[20];
    snprintf(label, sizeof(label), "10s #%d R%d", i + 1, snapshot.tenSecond[i].run);
    printSbpSpeedRange(
      label,
      sbpPath,
      snapshot.tenSecond[i].startSbp,
      snapshot.tenSecond[i].endSbp,
      snapshot.tenSecond[i].sumCms
    );
  }

  printSbpSpeedEdges("Alpha", sbpPath, snapshot.alpha.startSbp, snapshot.alpha.endSbp, 5);
  if (snapshot.alpha.startSbp >= 0 && snapshot.alpha.endSbp >= snapshot.alpha.startSbp) {
    Serial.printf(
      "Alpha detail: %.3f kn, %dm path, %.1fm closure\n",
      snapshot.alpha.speedKnots,
      snapshot.alpha.distanceM,
      snapshot.alpha.closureM
    );
  }

  Serial.println("========================================================");
#else
  (void)sbpPath;
  (void)snapshot;
#endif
}
