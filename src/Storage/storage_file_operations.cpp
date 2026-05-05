// -----------------------------------------------------------------------------
// File Operations Manager
// Opens / flushes / logs / closes UBX and SBP session files.
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <FS.h>
#include <esp_system.h>

#include "Core/Definitions.h"
#include "Core/Globals.h"
#include "Core/system_info.h"
#include "Storage/geojson.h"
#include "Storage/sbp.h"
#include "Storage/session_geojson.h"
#include "Storage/session_raw_writers.h"
#include "Storage/storage_file_operations.h"
#include "Storage/storage_manager.h"
#include "managers/config_types.h"

namespace {
char dataStr[255] = "";
char Buffer[50] = "";
uint64_t GPS_UTC_ms;

File ubxfile;
File sbpfile;

char filenameERR[128] = "/";
char filenameUBX[128] = "/";
char filenameSBP[128] = "/";
char filenameGEO[128] = "/";
}

void Open_files(void)
{
  if (storage_is_shutting_down() || !Time_Set_OK) {
    LOG_STORAGE("Open_files", "called without valid GPS time");
    return;
  }

  if (!storage_logs_dir_ready()) {
    LOG_ERROR("STORAGE", "logs dir unavailable");
    return;
  }

  fs::FS& storage = storage_sd_fs();
  getLocalTime(&tmstruct);

  uint64_t chipMac = 0;
  esp_efuse_mac_get_default((uint8_t*)&chipMac);

  uint8_t mac3 = (chipMac >> 16) & 0xFF;
  uint8_t mac4 = (chipMac >> 8) & 0xFF;
  uint8_t mac5 = chipMac & 0xFF;

  char base[96];
  char path[128];

  snprintf(base, sizeof(base),
    "%02X%02X%02X_%04d%02d%02d_%02d%02d%02d",
    mac3, mac4, mac5,
    tmstruct.tm_year + 1900,
    tmstruct.tm_mon + 1,
    tmstruct.tm_mday,
    tmstruct.tm_hour,
    tmstruct.tm_min,
    tmstruct.tm_sec
  );

  snprintf(path, sizeof(path), "/logs/%s", base);

  snprintf(filenameERR, sizeof(filenameERR), "%s.txt", path);
  snprintf(filenameUBX, sizeof(filenameUBX), "%s.ubx", path);
  snprintf(filenameSBP, sizeof(filenameSBP), "%s.sbp", path);
  snprintf(filenameGEO, sizeof(filenameGEO), "%s.geojson", path);

  if (config.logUBX) {
    ubxfile = storage.open(filenameUBX, FILE_APPEND);
  }

  if (config.logSBP) {
    sbpfile = storage.open(filenameSBP, FILE_WRITE);
    if (sbpfile.size() == 0) {
      log_header_SBP(sbpfile);
    }
  }

  geojson_begin(filenameGEO);
  geojson_begin_feature("track");

  LOG_STORAGE("LOG", "Session started %s", base);
}

void Flush_files(void)
{
  if (storage_is_shutting_down() || systemInfo.sample_rate > 10) return;

  static uint8_t lb = 0;
  switch (lb) {
    case 0:
      if (ubxfile) ubxfile.flush();
      break;

    case 3:
      if (sbpfile) sbpfile.flush();
      break;
  }

  lb = (lb + 1) % 5;
}

void Log_to_SD(void)
{
  if (storage_is_shutting_down() || !Time_Set_OK) return;

  session_write_ubx(ubxfile);
  session_write_sbp(sbpfile);
}

void Close_files(void)
{
  Serial.println("[STORAGE] Close_files()");

  session_geojson_finalize();

  if (ubxfile) {
    ubxfile.flush();
    ubxfile.close();
    ubxfile = File();
  }

  if (sbpfile) {
    sbpfile.flush();
    sbpfile.close();
    sbpfile = File();
  }

  Serial.println("[STORAGE] files closed");
}
