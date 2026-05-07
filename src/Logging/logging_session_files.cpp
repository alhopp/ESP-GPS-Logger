// -----------------------------------------------------------------------------
// File Operations Manager
// Opens / flushes / logs / closes UBX and SBP session files.
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <FS.h>
#include <esp_system.h>

#include "Core/log.h"
#include "Core/Globals.h"
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
