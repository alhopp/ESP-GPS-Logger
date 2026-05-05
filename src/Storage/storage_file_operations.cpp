// -----------------------------------------------------------------------------
// File Operations Manager
// Opens / flushes / logs / closes UBX and SBP session files.
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <FS.h>
#include <esp_system.h>

#include "Core/Definitions.h"
#include "Core/Globals.h"
#include "Storage/geojson.h"
#include "Storage/sbp.h"
#include "Storage/session_geojson.h"
#include "Storage/session_raw_writers.h"
#include "Storage/storage_file_operations.h"
#include "Storage/storage_manager.h"
#include "managers/config_types.h"

namespace {
File ubxfile;
File sbpfile;

void buildSessionBase(char* base, size_t baseSize)
{
  getLocalTime(&tmstruct);

  uint64_t chipMac = 0;
  esp_efuse_mac_get_default((uint8_t*)&chipMac);

  const uint8_t mac3 = (chipMac >> 16) & 0xFF;
  const uint8_t mac4 = (chipMac >> 8) & 0xFF;
  const uint8_t mac5 = chipMac & 0xFF;

  snprintf(base, baseSize,
    "%02X%02X%02X_%04d%02d%02d_%02d%02d%02d",
    mac3, mac4, mac5,
    tmstruct.tm_year + 1900,
    tmstruct.tm_mon + 1,
    tmstruct.tm_mday,
    tmstruct.tm_hour,
    tmstruct.tm_min,
    tmstruct.tm_sec
  );
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

bool storage_files_open()
{
  if (storage_is_shutting_down() || !Time_Set_OK) {
    LOG_STORAGE("storage_files_open", "called without valid GPS time");
    return false;
  }

  if (!storage_logs_dir_ready()) {
    LOG_ERROR("STORAGE", "logs dir unavailable");
    return false;
  }

  fs::FS& storage = storage_sd_fs();
  session_raw_writers_reset();

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

  if (config.logSBP) {
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
  }

  if (!geojson_begin(filenameGEO)) {
    LOG_ERROR("STORAGE", "GeoJSON open failed");
    closeFile(ubxfile);
    closeFile(sbpfile);
    return false;
  }

  geojson_begin_feature("track");

  LOG_STORAGE("LOG", "Session started %s", base);
  return true;
}

void storage_files_write_raw()
{
  if (storage_is_shutting_down() || !Time_Set_OK) return;

  session_raw_writers_write_ubx(ubxfile);
  session_raw_writers_write_sbp(sbpfile);
}

void storage_files_close()
{
  Serial.println("[STORAGE] storage_files_close()");

  session_geojson_finalize();

  closeFile(ubxfile);
  closeFile(sbpfile);

  Serial.println("[STORAGE] files closed");
}
