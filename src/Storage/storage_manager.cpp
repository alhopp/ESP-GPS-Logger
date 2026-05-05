// -----------------------------------------------------------------------------
// storage_manager.cpp
// ESP32 storage manager:
// - LittleFS = always-on control/config storage
// - SD_MMC   = removable/high-volume data logging storage
// -----------------------------------------------------------------------------

#include "storage/storage_manager.h"
#include "storage/storage_file_operations.h"

#include <SD_MMC.h>
#include <LittleFS.h>

#include "managers/config_manager.h"
#include "core/Definitions.h"

// Public state flags
bool sdOK=false, LITTLEFS_OK=false;
bool storage_shutting_down=false;

// Internal mount state
static bool sd_mounted=false;

// Internal helpers
static bool mountSD_MMC();
static void unmountSD_MMC();
static fs::FS& activeFS();
static bool quickIOTest(fs::FS& fs,const char* path);
static void logLittleFSStats();
static void logSDStats();

// -----------------------------------------------------------------------------
// Init both filesystems:
// - LittleFS is mounted first for config/control plane
// - SD_MMC is then mounted for logging/data plane
// -----------------------------------------------------------------------------
void initStorage()
{
  LOG_STORAGE("Init","start");
  sdOK=false; LITTLEFS_OK=false;

  // Mount LittleFS so config/control storage is always available.
  if(!LittleFS.begin(true)) LOG_ERROR("LittleFS","mount failed");
  else{
    LITTLEFS_OK=true;
    LOG_STORAGE("LittleFS","mounted");
    logLittleFSStats();
  }

  // Mount SD_MMC for main data logging if card is present/usable.
  if(mountSD_MMC()){
    sdOK=true;
    LOG_STORAGE("SD","MMC mounted");
    logSDStats();

    // Quick write/remove check to catch bad cards or broken mount states.
    if(quickIOTest(activeFS(),"/.io_test")) LOG_STORAGE("SD I/O","OK");
    else LOG_ERROR("SD I/O","FAILED");
  } else {
    LOG_STORAGE("SD","not available");
  }

  LOG_STORAGE("Init","done");
}

// Ensure SD_MMC is mounted before use.
// Returns false if SD was never successfully detected or remount fails.
bool storage_on()
{
  if(!sdOK) return false;
  if(!mountSD_MMC()) return false;
  return true;
}

// Unmount SD_MMC cleanly.
// Returns true to preserve current API shape.
bool storage_off()
{
  unmountSD_MMC();
  return true;
}

// Return preferred active filesystem:
// - SD_MMC when available
// - otherwise fall back to LittleFS
static fs::FS& activeFS(){ return sdOK ? static_cast<fs::FS&>(SD_MMC) : static_cast<fs::FS&>(LittleFS); }

// Mount SD_MMC in 1-bit safe mode.
// DAT0 gets a pull-up preflight to improve bring-up reliability.
static bool mountSD_MMC()
{
  if(sd_mounted) return true;

  LOG_STORAGE("SD MMC","preflight");
  pinMode(SDMMC_DAT0_PIN,INPUT_PULLUP);
  delay(2);

  LOG_STORAGE("SD MMC","init");
  if(!SD_MMC.begin(SD_MMC_MOUNTPOINT,SD_MMC_1BIT_MODE)){
    LOG_STORAGE("SD MMC","no card");
    sd_mounted=false;
    return false;
  }

  sd_mounted=true;
  return true;
}

// Clean shutdown of SD_MMC:
// - close open files first
// - end SD_MMC bus
// - short delay for stability before continuing
static void unmountSD_MMC()
{
  if(!sd_mounted) return;

  LOG_STORAGE("SD MMC","closing files");
  Close_files();

  LOG_STORAGE("SD MMC","unmount");
  SD_MMC.end();
  vTaskDelay(pdMS_TO_TICKS(300));
  sd_mounted=false;
}

// Simple write/flush/remove test to validate filesystem usability.
static bool quickIOTest(fs::FS& fs,const char* path)
{
  File f=fs.open(path,FILE_WRITE);
  if(!f) return false;
  f.println("ok");
  f.flush();
  f.close();
  fs.remove(path);
  return true;
}

// Log SD card capacity/usage in MB.
static void logSDStats()
{
  uint64_t total=SD_MMC.totalBytes(), used=SD_MMC.usedBytes(), freeb=(total>used)?(total-used):0;
  LOG_STORAGE("SD Total","%llu MB",(unsigned long long)(total/(1024ULL*1024ULL)));
  LOG_STORAGE("SD Used","%llu MB",(unsigned long long)(used /(1024ULL*1024ULL)));
  LOG_STORAGE("SD Free","%llu MB",(unsigned long long)(freeb/(1024ULL*1024ULL)));
}

// Log LittleFS capacity/usage in KB.
static void logLittleFSStats()
{
  LOG_STORAGE("LittleFS","total=%uKB used=%uKB free=%uKB",
    (unsigned)(LittleFS.totalBytes()/1024),
    (unsigned)(LittleFS.usedBytes()/1024),
    (unsigned)((LittleFS.totalBytes()-LittleFS.usedBytes())/1024));
}