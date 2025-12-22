#include "storage_manager.h"

#include <Arduino.h>
#include <SD_MMC.h>
#include <LittleFS.h>

#include "SD_card.h"
#include "ESP_functions.h"
#include "config_manager.h"

// ----------------------------------------------------
// External state
// ----------------------------------------------------
extern bool sdOK;
extern bool LITTLEFS_OK;

// ----------------------------------------------------
// Internal helpers
// ----------------------------------------------------
static void reportSDStats();
static bool mountSD();
static bool mountLittleFS();
static void storageBannerStart();
static void storageBannerEnd();
static bool storageQuickCheck(fs::FS &fs, const char *path);

// ----------------------------------------------------
// Public API
// ----------------------------------------------------
void initStorage()
{
  storageBannerStart();

  // ------------------------------
  // Try SD card first
  // ------------------------------
  if (mountSD()) {
    Serial.println("[STORAGE] SD_MMC mounted successfully");

    // Report SD statistics
    reportSDStats();

    if (!storageQuickCheck(SD_MMC, "/.io_test")) {
     Serial.println(F("[STORAGE] SD I/O        : failed"));
     } else {
     Serial.println(F("[STORAGE] SD I/O        : OK"));
    }

    storageBannerEnd();
    return;
  }

  // ------------------------------
  // Fallback to LittleFS
  // ------------------------------
  Serial.println("[STORAGE] No SD card found — trying LittleFS");

  if (mountLittleFS()) {
    Serial.println("[STORAGE] LittleFS mounted successfully");

    size_t total = LittleFS.totalBytes();
    size_t used  = LittleFS.usedBytes();

    Serial.printf(
      "[STORAGE] LittleFS total: %u KB, used: %u KB, free: %u KB\n",
      used  / 1024,
      (total - used) / 1024
    );

    Serial.println("[STORAGE] Using LittleFS");
  } else {
    Serial.println("[STORAGE] ERROR: LittleFS mount failed");
  }
}


// ----------------------------------------------------
// Helpers
// ----------------------------------------------------

static void storageBannerStart()
{
  Serial.println(F("[STORAGE] ****************************"));
  Serial.println(F("[STORAGE] *        STORAGE INIT      *"));
  Serial.println(F("[STORAGE] ****************************"));
}

static void storageBannerEnd()
{
  Serial.println(F("[STORAGE] ****************************"));
}

static bool mountSD()
{
  if (!SD_MMC.begin("/sdcard", true)) {
    sdOK = false;
    return false;
  }

  sdOK = true;
  Serial.println(F("[STORAGE] SDCard found"));
  return true;
}

static bool mountLittleFS()
{
  if (!LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    LITTLEFS_OK = false;
    return false;
  }

  LITTLEFS_OK = true;
  Serial.println(F("[STORAGE] SDCard found"));
  Serial.print(F("[STORAGE] LITTLEFS mounted, total bytes = "));
  Serial.println(LITTLEFS.totalBytes());
  return true;
}

static bool storageQuickCheck(fs::FS &fs, const char *path)
{
  File f = fs.open(path, FILE_WRITE);
  if (!f) {
    return false;
  }

  f.println("ok");
  f.flush();
  f.close();

  fs.remove(path);
  return true;
}

uint64_t storageFreeKBytes()
{
  if (sdOK) {
    return (SD_MMC.totalBytes() - SD_MMC.usedBytes()) / 1024;
  }

  if (LITTLEFS_OK) {
    return (LITTLEFS.totalBytes() - LITTLEFS.usedBytes()) / 1024;
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





static void reportSDStats()
{
  // Work in BYTES first (always safe)
  uint64_t card_bytes  = SD_MMC.cardSize();
  uint64_t total_bytes = SD_MMC.totalBytes();
  uint64_t used_bytes  = SD_MMC.usedBytes();
  uint64_t free_bytes  = total_bytes - used_bytes;

  // Convert to MB explicitly
  uint32_t card_mb  = card_bytes  / (1024ULL * 1024ULL);
  uint32_t total_mb = total_bytes / (1024ULL * 1024ULL);
  uint32_t used_mb  = used_bytes  / (1024ULL * 1024ULL);
  uint32_t free_mb  = free_bytes  / (1024ULL * 1024ULL);

  Serial.printf("[STORAGE] SD Card Size  : %lu MB\n", card_mb);
  Serial.printf("[STORAGE] SD Total      : %lu MB\n", total_mb);
  Serial.printf("[STORAGE] SD Used       : %lu MB\n", used_mb);
  Serial.printf("[STORAGE] SD Free       : %lu MB\n", free_mb);
}
