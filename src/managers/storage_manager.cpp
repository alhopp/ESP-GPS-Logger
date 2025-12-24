// -----------------------------------------------------------------------------
// storage_manager.cpp
//
// SIMPLE storage initialization:
// - SD card is ASSUMED PRESENT
// - SD_MMC is initialised unconditionally (1-bit mode)
// - LittleFS is also mounted (mandatory fallback / config)
// - sdOK reflects SD_MMC.begin() result only
//
// NOTE:
//   If no SD card is present, SD_MMC.begin() MAY BLOCK.
//   We will handle SD-missing cases later.
// -----------------------------------------------------------------------------

#include "storage_manager.h"

#include <Arduino.h>
#include <SD_MMC.h>
#include <LittleFS.h>

#include "SD_card.h"
#include "ESP_functions.h"
#include "config_manager.h"
#include "Definitions.h"

// -----------------------------------------------------------------------------
// GLOBAL STORAGE STATE (DEFINED ONCE HERE)
// -----------------------------------------------------------------------------
bool sdOK        = false;
bool LITTLEFS_OK = false;

// -----------------------------------------------------------------------------
// INTERNAL HELPERS
// -----------------------------------------------------------------------------
static bool mountSD();
static bool mountLittleFS();
static bool storageQuickCheck(fs::FS &fs, const char *path);
static void reportSDStats();

// -----------------------------------------------------------------------------
// PUBLIC API
// -----------------------------------------------------------------------------
void initStorage()
{
  LOG_STORAGE("Init", "start");

  // ---------------------------------------------------------------------------
  // SD CARD (ASSUMED PRESENT)
  // ---------------------------------------------------------------------------
  if (mountSD()) {
    LOG_STORAGE("SD", "mounted");
    reportSDStats();

    if (storageQuickCheck(SD_MMC, "/.io_test")) {
      LOG_STORAGE("SD I/O", "OK");
    } else {
      LOG_ERROR("SD I/O", "FAILED");
    }
  } else {
    LOG_ERROR("SD", "mount failed");
  }

  // ---------------------------------------------------------------------------
  // LITTLEFS (MANDATORY)
  // ---------------------------------------------------------------------------
  if (!mountLittleFS()) {
    LOG_ERROR("LittleFS", "unavailable");
  }

  LOG_STORAGE("Init", "done");
}

// -----------------------------------------------------------------------------
// SPACE HELPERS
// -----------------------------------------------------------------------------
uint64_t storageFreeKBytes()
{
  if (sdOK) {
    return (SD_MMC.totalBytes() - SD_MMC.usedBytes()) / 1024ULL;
  }

  if (LITTLEFS_OK) {
    return (LittleFS.totalBytes() - LittleFS.usedBytes()) / 1024ULL;
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

  if (data_rate <= 0) return 0;

  uint64_t seconds = (free_kbytes * 1024ULL) / data_rate;
  return seconds / 60;
}

// -----------------------------------------------------------------------------
// INTERNAL IMPLEMENTATION
// -----------------------------------------------------------------------------
static bool mountSD()
{
  sdOK = false;

  // ---------------------------------------------------------------------------
  // ASSUME SD IS PRESENT
  // 1-bit SDMMC mode (safe for LilyGO / T5)
  // ---------------------------------------------------------------------------
  if (!SD_MMC.begin("/sdcard", true)) {
    LOG_ERROR("SD", "SD_MMC.begin failed");
    return false;
  }

  sdOK = true;
  return true;
}

static bool mountLittleFS()
{
  if (!LittleFS.begin(true)) {   // format if corrupt
    LITTLEFS_OK = false;
    LOG_ERROR("LittleFS", "mount failed");
    return false;
  }

  LITTLEFS_OK = true;

  LOG_STORAGE(
    "LittleFS",
    "total=%uKB used=%uKB free=%uKB",
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

  LOG_STORAGE("SD Size",  "%lu MB", card_bytes  / (1024ULL * 1024ULL));
  LOG_STORAGE("SD Total", "%lu MB", total_bytes / (1024ULL * 1024ULL));
  LOG_STORAGE("SD Used",  "%lu MB", used_bytes  / (1024ULL * 1024ULL));
  LOG_STORAGE("SD Free",  "%lu MB", free_bytes  / (1024ULL * 1024ULL));
}
