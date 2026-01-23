// -----------------------------------------------------------------------------
// storage_manager.cpp
//
// ESP32 storage manager (SD_MMC + LittleFS)
// - LittleFS: always-on control plane
// - SD_MMC: data plane (1-bit safe mode)
// -----------------------------------------------------------------------------

#include "Storage/storage_manager.h"
#include "Storage/storage_file_operations.h"

#include <SD_MMC.h>
#include <LittleFS.h>

#include "config_manager.h"
#include "Definitions.h"

// -----------------------------------------------------------------------------
// Globals (extern)
// -----------------------------------------------------------------------------
bool sdOK=false, LITTLEFS_OK=false;
volatile bool storage_shutting_down=false;

// -----------------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------------
static bool sd_mounted=false;

// -----------------------------------------------------------------------------
// Forward decls
// -----------------------------------------------------------------------------
static bool mountSD_MMC();
static void unmountSD_MMC();
static fs::FS& activeFS();
static bool quickIOTest(fs::FS& fs,const char* path);
static void logLittleFSStats(), logSDStats();


// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------
void initStorage()
{
  LOG_STORAGE("Init","start");
  sdOK=false; LITTLEFS_OK=false;

  // LittleFS (control plane)
  if(!LittleFS.begin(true)) LOG_ERROR("LittleFS","mount failed");
  else{ LITTLEFS_OK=true; LOG_STORAGE("LittleFS","mounted"); logLittleFSStats(); }

  // SD_MMC (data plane)
  if(mountSD_MMC()){
    sdOK=true; LOG_STORAGE("SD","MMC mounted");
    logSDStats();
    if(quickIOTest(activeFS(),"/.io_test")) LOG_STORAGE("SD I/O","OK");
    else LOG_ERROR("SD I/O","FAILED");
  } else LOG_STORAGE("SD","not available");

  LOG_STORAGE("Init","done");
}

bool storage_on()
{
  if (!sdOK) return false;
  if (!mountSD_MMC()) return false;
  
  return true;
}



void storage_off(){ unmountSD_MMC(); }

// -----------------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------------
static fs::FS& activeFS(){if (sdOK) return SD_MMC; return LittleFS;}

static bool mountSD_MMC()
{
  if(sd_mounted) return true;

  LOG_STORAGE("SD MMC","preflight");
  pinMode(SDMMC_DAT0_PIN,INPUT_PULLUP); delay(2);

  LOG_STORAGE("SD MMC","init");
  if(!SD_MMC.begin(SD_MMC_MOUNTPOINT,SD_MMC_1BIT_MODE)){
    LOG_STORAGE("SD MMC","no card"); sd_mounted=false; return false;
  }

  sd_mounted=true; return true;
}

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

static bool quickIOTest(fs::FS& fs,const char* path)
{
  File f=fs.open(path,FILE_WRITE);
  if(!f) return false;
  f.println("ok"); f.flush(); f.close();
  fs.remove(path); return true;
}

static void logSDStats()
{
  uint64_t total=SD_MMC.totalBytes(), used=SD_MMC.usedBytes();
  uint64_t freeb=(total>used)?(total-used):0;

  LOG_STORAGE("SD Total","%llu MB",(unsigned long long)(total/(1024ULL*1024ULL)));
  LOG_STORAGE("SD Used","%llu MB",(unsigned long long)(used /(1024ULL*1024ULL)));
  LOG_STORAGE("SD Free","%llu MB",(unsigned long long)(freeb/(1024ULL*1024ULL)));
}

static void logLittleFSStats()
{
  LOG_STORAGE("LittleFS","total=%uKB used=%uKB free=%uKB",
    (unsigned)(LittleFS.totalBytes()/1024),
    (unsigned)(LittleFS.usedBytes()/1024),
    (unsigned)((LittleFS.totalBytes()-LittleFS.usedBytes())/1024));
}

