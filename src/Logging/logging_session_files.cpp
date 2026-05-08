// -----------------------------------------------------------------------------
// File Operations Manager
// Opens / flushes / logs / closes UBX and SBP session files.
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <FS.h>
#include <esp_system.h>

#include "Core/log.h"
#include "Core/Globals.h"
#include "Core/build_config.h"
#include "Core/system_info.h"
#include "GPS/gps_config.h"
#include "GPS/Metrics/gps_alpha_speed.h"
#include "GPS/Metrics/gps_time_speed.h"
#include "Logging/sbp_writer.h"
#include "Logging/geojson_session_export.h"
#include "Logging/logging_raw_writers.h"
#include "Logging/logging_session_files.h"
#include "Storage/storage_manager.h"
#include "Config/config_types.h"

namespace {
File ubxfile;
File sbpfile;
char activeSbpPath[128];
char activeGeoPath[128];

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

const char* basenameOnly(const char* path)
{
  if (!path) return nullptr;
  const char* p = strrchr(path, '/');
  return p ? p + 1 : path;
}

int sessionNumberFromName(const char* name, const char* datePrefix, const char* deviceId)
{
  if (!name || !datePrefix || !deviceId) return 0;

  char expectedDevice[16];
  snprintf(expectedDevice, sizeof(expectedDevice), "_%s.", deviceId);
  if (!strstr(name, expectedDevice)) return 0;

  const size_t dateLen = strlen(datePrefix);
  if (strncmp(name, datePrefix, dateLen) != 0) return 0;
  if (name[dateLen] != '_' || name[dateLen + 1] != 'S') return 0;

  int sessionNumber = 0;
  if (sscanf(name + dateLen + 2, "%d", &sessionNumber) != 1) return 0;
  return sessionNumber > 0 ? sessionNumber : 0;
}

int nextSessionNumberForToday(const char* datePrefix, const char* deviceId)
{
  fs::FS& storage = storage_sd_fs();
  File dir = storage.open("/logs");
  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    return 1;
  }

  int maxSession = 0;
  File file = dir.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      const int sessionNumber =
          sessionNumberFromName(basenameOnly(file.name()), datePrefix, deviceId);
      if (sessionNumber > maxSession) maxSession = sessionNumber;
    }
    file.close();
    file = dir.openNextFile();
  }
  dir.close();

  return maxSession + 1;
}

void buildSessionBase(char* base, size_t baseSize)
{
  getLocalTime(&tmstruct);

  uint64_t chipMac = 0;
  esp_efuse_mac_get_default((uint8_t*)&chipMac);

  const uint8_t mac3 = (chipMac >> 16) & 0xFF;
  const uint8_t mac4 = (chipMac >> 8) & 0xFF;
  const uint8_t mac5 = chipMac & 0xFF;

  char deviceId[8];
  snprintf(deviceId, sizeof(deviceId), "%02X%02X%02X", mac3, mac4, mac5);

  char datePrefix[16];
  snprintf(datePrefix, sizeof(datePrefix),
           "%04d-%02d-%02d",
           tmstruct.tm_year + 1900,
           tmstruct.tm_mon + 1,
           tmstruct.tm_mday);

  const int sessionNumber = nextSessionNumberForToday(datePrefix, deviceId);

  snprintf(base, baseSize,
           "%s_S%02d_%s",
           datePrefix,
           sessionNumber,
           deviceId);
}

void buildSessionPath(char* path, size_t pathSize, const char* base, const char* extension)
{
  snprintf(path, pathSize, "/logs/%s.%s", base, extension);
}

void closeFile(File& file)
{
  if (!file) return;

  file.flush();
  file.close();
  file = File();
}

int sampleRate()
{
  return systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 1;
}

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

void printSbpDebugSamples(const char* sbpPath)
{
#if LOG_ENABLED
  if (!sbpPath || !sbpPath[0]) return;

  gps_time_speed_rebuild_10s_top5_per_run();

  const int rate = sampleRate();

  Serial.println();
  Serial.println("================ SBP STAT SAMPLE VALUES ================");

  printSbpSpeedRange(
    "2s",
    sbpPath,
    win_2s_sbp_start,
    win_2s_sbp_start >= 1 ? win_2s_sbp_start + (2 * rate) - 1 : -1,
    win_2s_sum_cms
  );

  for (int i = 0; i < 5; i++) {
    char label[20];
    snprintf(label, sizeof(label), "10s #%d R%d", i + 1, win_10s_top5_run[i]);
    printSbpSpeedRange(
      label,
      sbpPath,
      win_10s_top5_sbp_start[i],
      win_10s_top5_sbp_start[i] >= 1 ? win_10s_top5_sbp_start[i] + (10 * rate) - 1 : -1,
      win_10s_top5_sum_cms[i]
    );
  }

  printSbpSpeedEdges("Alpha", sbpPath, alpha_sbp_start, alpha_sbp_end, 5);
  if (alpha_sbp_start >= 0 && alpha_sbp_end >= alpha_sbp_start) {
    Serial.printf(
      "Alpha detail: %.3f kn, %dm path, %.1fm closure\n",
      alpha_best_speed_mmps * MMPS_TO_KNOTS,
      alpha_best_distance_m,
      alpha_best_closure_m
    );
  }

  Serial.println("========================================================");
#else
  (void)sbpPath;
#endif
}
}

bool logging_session_files_open()
{
  if (storage_is_shutting_down() || !Time_Set_OK) {
    LOG_STORAGE("session_files", "called without valid GPS time");
    return false;
  }

  if (!storage_logs_dir_ready()) {
    LOG_ERROR("STORAGE", "logs dir unavailable");
    return false;
  }

  fs::FS& storage = storage_sd_fs();
  logging_raw_writers_reset();
  activeSbpPath[0] = '\0';
  activeGeoPath[0] = '\0';

  char base[96];
  char filenameUBX[128];
  char filenameSBP[128];
  char filenameGEO[128];

  buildSessionBase(base, sizeof(base));
  buildSessionPath(filenameUBX, sizeof(filenameUBX), base, "ubx");
  buildSessionPath(filenameSBP, sizeof(filenameSBP), base, "sbp");
  buildSessionPath(filenameGEO, sizeof(filenameGEO), base, "geojson");

  if (config.logUBX) {
    ubxfile = storage.open(filenameUBX, FILE_APPEND);
    if (!ubxfile) {
      LOG_ERROR("STORAGE", "UBX open failed");
      closeFile(ubxfile);
      return false;
    }
  }

  sbpfile = storage.open(filenameSBP, FILE_WRITE);
  if (!sbpfile) {
    LOG_ERROR("STORAGE", "SBP open failed");
    closeFile(ubxfile);
    closeFile(sbpfile);
    return false;
  }

  if (sbpfile.size() == 0) {
    sbp_write_header(sbpfile);
  }

  strlcpy(activeSbpPath, filenameSBP, sizeof(activeSbpPath));
  strlcpy(activeGeoPath, filenameGEO, sizeof(activeGeoPath));

  LOG_STORAGE("LOG", "Session started %s", base);
  return true;
}

void logging_session_files_write_raw()
{
  if (storage_is_shutting_down() || !Time_Set_OK) return;

  logging_raw_writers_write_ubx(ubxfile);
  logging_raw_writers_write_sbp(sbpfile);
}

void logging_session_files_close()
{
  Serial.println("[STORAGE] logging_session_files_close()");

  closeFile(sbpfile);

  if (activeSbpPath[0] != '\0' && activeGeoPath[0] != '\0') {
    printSbpDebugSamples(activeSbpPath);

    if (!geojson_session_export_finalize(activeSbpPath, activeGeoPath)) {
      LOG_ERROR("STORAGE", "GeoJSON export failed");
    }
  }

  closeFile(ubxfile);
  closeFile(sbpfile);
  activeSbpPath[0] = '\0';
  activeGeoPath[0] = '\0';

  Serial.println("[STORAGE] files closed");
}
