#include "Logging/logging_session_paths.h"

#include <Arduino.h>
#include <FS.h>
#include <esp_system.h>

#include "Core/Globals.h"
#include "Storage/storage_manager.h"

namespace {

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

}

void logging_session_paths_build(SessionPaths& paths)
{
  buildSessionBase(paths.base, sizeof(paths.base));
  buildSessionPath(paths.ubx, sizeof(paths.ubx), paths.base, "ubx");
  buildSessionPath(paths.sbp, sizeof(paths.sbp), paths.base, "sbp");
  buildSessionPath(paths.geojson, sizeof(paths.geojson), paths.base, "geojson");
}
