#include "storage_manager.h"

#include <Arduino.h>
#include <SD_MMC.h>
#include <LittleFS.h>

#include "SD_card.h"
#include "ESP_functions.h"
#include "config_manager.h"

// ----------------------------------------------------
// Global storage state (DEFINED ONCE HERE)
// ----------------------------------------------------
bool sdOK = false;
bool LITTLEFS_OK = false;

// ----------------------------------------------------
// Internal helpers
// ----------------------------------------------------
static void storageBannerStart();
static void storageBannerEnd();
static bool mountSD();
static bool mountLittleFS();
static void reportSDStats();
static bool storageQuickCheck(fs::FS &fs, const char *path);

// ----------------------------------------------------
// Public API
// ----------------------------------------------------
void initStorage()
{
  storageBannerStart();

  // ------------------------------
  // SD Card (optional)
  // ------------------------------
  if (mountSD()) {
    Serial.println("[STORAGE] SD_MMC mounted successfully");
    reportSDStats();

    if (storageQuickCheck(SD_MMC, "/.io_test")) {
      Serial.println("[STORAGE] SD I/O        : OK");
    } else {
      Serial.println("[STORAGE] SD I/O        : FAILED");
    }
  } else {
    Serial.println("[STORAGE] No SD card detected");
  }

  // ------------------------------
  // LittleFS (mandatory)
  // ------------------------------
  if (!mountLittleFS()) {
    Serial.println("[STORAGE] FATAL: LittleFS unavailable");
  }

  storageBannerEnd();
}

// ----------------------------------------------------
// Space helpers
// ----------------------------------------------------
uint64_t storageFreeKBytes()
{
  if (sdOK) {
    return (SD_MMC.totalBytes() - SD_MMC.usedBytes()) / 1024;
  }

  if (LITTLEFS_OK) {
    return (LittleFS.totalBytes() - LittleFS.usedBytes()) / 1024;
  }

  return 0;
}

int storageLogTimeLeftMinutes()
{
  uint64_t free_kbytes = storageFreeKBytes();
  if (free_kbytes == 0) return 0;

  int data_rate =
      (config.logGPY * 24 +
       config.logUBX * 100 +
       config.logSBP * 32 +
       1) * config.sample_rate +
      config.logGPX * 230;

  uint64_t seconds = (free_kbytes * 1024ULL) / data_rate;
  return seconds / 60;
}

// ----------------------------------------------------
// Internal helpers
// ----------------------------------------------------
static bool mountSD()
{
  if (!SD_MMC.begin("/sdcard", true)) {
    sdOK = false;
    return false;
  }

  sdOK = true;
  Serial.println("[STORAGE] SD card detected");
  return true;
}

static bool mountLittleFS()
{
  if (!LittleFS.begin(true)) {   // format if corrupt
    LITTLEFS_OK = false;
    Serial.println("[STORAGE] LittleFS mount failed");
    return false;
  }

  LITTLEFS_OK = true;

  Serial.printf(
    "[STORAGE] LittleFS total: %u KB, used: %u KB, free: %u KB\n",
    LittleFS.totalBytes() / 1024,
    LittleFS.usedBytes()  / 1024,
    (LittleFS.totalBytes() - LittleFS.usedBytes()) / 1024
  );

  return true;
}

static bool storageQuickCheck(fs::FS &fs, const char *path)
{
  File f = fs.open(path, FILE_WRITE);
  if (!f) return false;

  f.println("ok");
  f.flush();
  f.close();

  fs.remove(path);
  return true;
}

static void reportSDStats()
{
  uint64_t card_bytes  = SD_MMC.cardSize();
  uint64_t total_bytes = SD_MMC.totalBytes();
  uint64_t used_bytes  = SD_MMC.usedBytes();
  uint64_t free_bytes  = total_bytes - used_bytes;

  Serial.printf("[STORAGE] SD Card Size  : %lu MB\n", card_bytes  / (1024ULL * 1024ULL));
  Serial.printf("[STORAGE] SD Total      : %lu MB\n", total_bytes / (1024ULL * 1024ULL));
  Serial.printf("[STORAGE] SD Used       : %lu MB\n", used_bytes  / (1024ULL * 1024ULL));
  Serial.printf("[STORAGE] SD Free       : %lu MB\n", free_bytes  / (1024ULL * 1024ULL));
}

static void storageBannerStart()
{
  Serial.println("[STORAGE] ****************************");
  Serial.println("[STORAGE] *        STORAGE INIT      *");
  Serial.println("[STORAGE] ****************************");
}

static void storageBannerEnd()
{
  Serial.println("[STORAGE] ****************************");
}
