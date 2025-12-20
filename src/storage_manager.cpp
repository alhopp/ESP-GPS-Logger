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
  const uint64_t cardSizeMB = SD_MMC.cardSize()   / (1024 * 1024);
  const uint64_t totalMB   = SD_MMC.totalBytes() / (1024 * 1024);
  const uint64_t usedMB    = SD_MMC.usedBytes()  / (1024 * 1024);

  freeSpace = totalMB - usedMB;

  Serial.printf("SD Card Size  : %llu MB\n", cardSizeMB);
  Serial.printf("SD Total      : %llu MB\n", totalMB);
  Serial.printf("SD Used       : %llu MB\n", usedMB);
  Serial.printf("SD Free       : %llu MB\n", freeSpace);
}
