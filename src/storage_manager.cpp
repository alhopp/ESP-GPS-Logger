#include "storage_manager.h"

#include <Arduino.h>
#include <SD_MMC.h>
#include <LittleFS.h>

#include "SD_card.h"
#include "ESP_functions.h"

// ----------------------------------------------------
// External state
// ----------------------------------------------------
extern bool sdOK;
extern bool LITTLEFS_OK;
extern int  freeSpace;

// ----------------------------------------------------
// Internal helpers
// ----------------------------------------------------
static void reportSDStats();
static bool mountSD();
static bool mountLittleFS();

// ----------------------------------------------------
// Public API
// ----------------------------------------------------
void initStorage()
{
  // Try SD card first
  if (mountSD()) {
    reportSDStats();
    testFileIO(SD_MMC, "/test.txt");
    return;
  }

  // Fallback to LittleFS
  Serial.println(F("No SDCard found — trying LITTLEFS"));

  if (!mountLittleFS()) {
    Serial.println(F("LITTLEFS mount failed"));
  }
}

// ----------------------------------------------------
// Helpers
// ----------------------------------------------------
static bool mountSD()
{
  if (!SD_MMC.begin("/sdcard", true)) {
    sdOK = false;
    return false;
  }

  sdOK = true;
  Serial.println(F("SDCard found"));
  return true;
}

static bool mountLittleFS()
{
  if (!LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    LITTLEFS_OK = false;
    return false;
  }

  LITTLEFS_OK = true;
  Serial.print(F("LITTLEFS mounted, total bytes = "));
  Serial.println(LITTLEFS.totalBytes());
  return true;
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

  // Store free space in MB (matches usage elsewhere)
  freeSpace = free_mb;

  Serial.printf("SD Card Size  : %lu MB\n", card_mb);
  Serial.printf("SD Total      : %lu MB\n", total_mb);
  Serial.printf("SD Used       : %lu MB\n", used_mb);
  Serial.printf("SD Free       : %lu MB\n", free_mb);
}
