// -----------------------------------------------------------------------------
// storage_manager.cpp
//
// Robust storage manager for ESP32
// - SD over SPI (preferred)
// - SD_MMC (optional fallback)
// - LittleFS always available
//
// FIXES:
// - fs::FS does NOT expose totalBytes()/usedBytes()
// - Backend-specific accounting is required
// -----------------------------------------------------------------------------

#include "storage_manager.h"

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <SD_MMC.h>
#include <LittleFS.h>

#include "ESP_functions.h"
#include "config_manager.h"
#include "Definitions.h"

// -----------------------------------------------------------------------------
// SD SPI PIN MAP
// -----------------------------------------------------------------------------
#define SD_SPI_CS    13
#define SD_SPI_SCK   14
#define SD_SPI_MOSI  15
#define SD_SPI_MISO  2

// -----------------------------------------------------------------------------
// CONFIG
// -----------------------------------------------------------------------------
#define ENABLE_SD_SPI   1
#define ENABLE_SD_MMC   1   // disable if unwanted

// -----------------------------------------------------------------------------
// STORAGE BACKEND TYPE
// -----------------------------------------------------------------------------
enum StorageBackend {
  STORAGE_NONE,
  STORAGE_SD_SPI,
  STORAGE_SD_MMC
};

// -----------------------------------------------------------------------------
// GLOBAL STATE
// -----------------------------------------------------------------------------
bool sdOK        = false;
bool LITTLEFS_OK = false;

static StorageBackend activeBackend = STORAGE_NONE;
static SPIClass sdSPI(VSPI);

// -----------------------------------------------------------------------------
// INTERNAL HELPERS
// -----------------------------------------------------------------------------
static bool mountSD_SPI();
static bool mountSD_MMC();
static bool mountLittleFS();
static bool storageQuickCheck(fs::FS& fs, const char* path);
static void reportSDStats();

// -----------------------------------------------------------------------------
// PUBLIC API
// -----------------------------------------------------------------------------
void initStorage()
{
  LOG_STORAGE("Init", "start");

  sdOK = false;
  activeBackend = STORAGE_NONE;

#if ENABLE_SD_SPI
  if (mountSD_SPI()) {
    sdOK = true;
    activeBackend = STORAGE_SD_SPI;
    LOG_STORAGE("SD", "SPI mounted");
  }
#endif

#if ENABLE_SD_MMC
  if (!sdOK && mountSD_MMC()) {
    sdOK = true;
    activeBackend = STORAGE_SD_MMC;
    LOG_STORAGE("SD", "MMC mounted");
  }
#endif

  if (sdOK) {
    reportSDStats();

 if (sdOK && storageQuickCheck(SD_MMC, "/.io_test")) {

      LOG_STORAGE("SD I/O", "OK");
    } else {
      LOG_ERROR("SD I/O", "FAILED");
    }
  } else {
    LOG_STORAGE("SD", "not available");
  }

  if (!mountLittleFS()) {
    LOG_ERROR("LittleFS", "mount failed");
  }

  LOG_STORAGE("Init", "done");
}

// -----------------------------------------------------------------------------
// FILESYSTEM ACCESS
// -----------------------------------------------------------------------------
fs::FS& storageFS()
{
  if (sdOK) {
    if (activeBackend == STORAGE_SD_SPI)  return SD;
    if (activeBackend == STORAGE_SD_MMC)  return SD_MMC;
  }
  return LittleFS;
}

bool storageHasSD()
{
  return sdOK;
}

// -----------------------------------------------------------------------------
// SPACE HELPERS (BACKEND-SAFE)
// -----------------------------------------------------------------------------
uint64_t storageFreeKBytes()
{
  uint64_t total = 0;
  uint64_t used  = 0;

  if (sdOK) {
    if (activeBackend == STORAGE_SD_SPI) {
      total = SD.totalBytes();
      used  = SD.usedBytes();
    } else if (activeBackend == STORAGE_SD_MMC) {
      total = SD_MMC.totalBytes();
      used  = SD_MMC.usedBytes();
    }
  } else if (LITTLEFS_OK) {
    total = LittleFS.totalBytes();
    used  = LittleFS.usedBytes();
  }

  if (total <= used) return 0;
  return (total - used) / 1024ULL;
}

int storageLogTimeLeftMinutes()
{
  uint64_t free_kbytes = storageFreeKBytes();
  if (!free_kbytes) return 0;

  int data_rate =
      (config.logGPY * 24 +
       config.logUBX * 100 +
       config.logSBP * 32 +
       1) * config.sample_rate +
      config.logGPX * 230;

  if (data_rate <= 0) return 0;
  return ((free_kbytes * 1024ULL) / data_rate) / 60;
}

// -----------------------------------------------------------------------------
// INTERNAL IMPLEMENTATION
// -----------------------------------------------------------------------------
static bool mountSD_SPI()
{
  LOG_STORAGE("SD SPI", "init");

  sdSPI.begin(SD_SPI_SCK, SD_SPI_MISO, SD_SPI_MOSI, SD_SPI_CS);

  if (!SD.begin(SD_SPI_CS, sdSPI, 25000000)) {
    LOG_ERROR("SD SPI", "begin failed");
    return false;
  }

  return true;
}

static bool mountSD_MMC()
{
  LOG_STORAGE("SD MMC", "init");

  if (!SD_MMC.begin("/sdcard", true)) {
    LOG_ERROR("SD MMC", "begin failed");
    return false;
  }

  return true;
}

static bool mountLittleFS()
{
  if (!LittleFS.begin(true)) {
    LITTLEFS_OK = false;
    return false;
  }

  LITTLEFS_OK = true;

  LOG_STORAGE(
    "LittleFS",
    "total=%uKB used=%uKB free=%uKB",
    LittleFS.totalBytes() / 1024,
    LittleFS.usedBytes() / 1024,
    (LittleFS.totalBytes() - LittleFS.usedBytes()) / 1024
  );

  return true;
}

static bool storageQuickCheck(fs::FS& fs, const char* path)
{
  File f = fs.open(path, FILE_WRITE);
  if (!f) return false;

  f.println("ok");
  f.close();
  fs.remove(path);
  return true;
}

static void reportSDStats()
{
  uint64_t total = 0;
  uint64_t used  = 0;

  if (activeBackend == STORAGE_SD_SPI) {
    total = SD.totalBytes();
    used  = SD.usedBytes();
  } else if (activeBackend == STORAGE_SD_MMC) {
    total = SD_MMC.totalBytes();
    used  = SD_MMC.usedBytes();
  }

  LOG_STORAGE("SD Total", "%lu MB", total / (1024ULL * 1024ULL));
  LOG_STORAGE("SD Used",  "%lu MB", used  / (1024ULL * 1024ULL));
  LOG_STORAGE("SD Free",  "%lu MB", (total - used) / (1024ULL * 1024ULL));
}
