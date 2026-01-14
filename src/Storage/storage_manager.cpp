// -----------------------------------------------------------------------------
// storage_manager.cpp
//
// Robust storage manager for ESP32
// - SD_MMC first (SDNAND / SD slot, 1-bit safe mode)
// - LittleFS always available (control-plane fallback)
// -----------------------------------------------------------------------------

#include "Storage/storage_manager.h"
#include "Storage/storage_file_operations.h"

#include <SD.h>
#include <SD_MMC.h>
#include <LittleFS.h>

#include "config_manager.h"
#include "Definitions.h"

#ifndef ENABLE_SD_MMC
#define ENABLE_SD_MMC 1
#endif
#ifndef ENABLE_SD_SPI
#define ENABLE_SD_SPI 1
#endif

// -----------------------------------------------------------------------------
// Backend state
// -----------------------------------------------------------------------------
enum class StorageBackend : uint8_t { NONE=0, SD_MMC, SD_SPI };

// -----------------------------------------------------------------------------
// Globals (extern)
// -----------------------------------------------------------------------------
bool sdOK=false, LITTLEFS_OK=false;
volatile bool storage_shutting_down=false;

// -----------------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------------
static StorageBackend s_backend=StorageBackend::NONE;
static SPIClass s_sdSPI(VSPI);
static bool sd_mounted=false;

// -----------------------------------------------------------------------------
// Forward decls
// -----------------------------------------------------------------------------
static bool mountSD_MMC(), mountSD_SPI();
static void unmountSD_MMC();
static fs::FS& activeFS();
static bool quickIOTest(fs::FS& fs,const char* path);
static void logLittleFSStats(), logSDStats();
static void sdBytes(uint64_t& total,uint64_t& used);

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------
void initStorage()
{
  LOG_STORAGE("Init","start");
  sdOK=false; LITTLEFS_OK=false; s_backend=StorageBackend::NONE;

  // LittleFS always first (control plane)
  if(!LittleFS.begin(true)) LOG_ERROR("LittleFS","mount failed");
  else{ LITTLEFS_OK=true; LOG_STORAGE("LittleFS","mounted"); logLittleFSStats(); }

#if ENABLE_SD_MMC
  if(mountSD_MMC()){ sdOK=true; s_backend=StorageBackend::SD_MMC; LOG_STORAGE("SD","MMC mounted"); }
#endif

#if ENABLE_SD_SPI
  if(!sdOK && mountSD_SPI()){ sdOK=true; s_backend=StorageBackend::SD_SPI; LOG_STORAGE("SD","SPI mounted"); }
#endif

  if(sdOK){
    logSDStats();
    if(quickIOTest(activeFS(),"/.io_test")) LOG_STORAGE("SD I/O","OK");
    else LOG_ERROR("SD I/O","FAILED");
  } else LOG_STORAGE("SD","not available");

  LOG_STORAGE("Init","done");
}

bool storage_on()
{
  if(!sdOK) return false;
  if(s_backend==StorageBackend::SD_MMC) return mountSD_MMC();
  if(s_backend==StorageBackend::SD_SPI) return true;
  return false;
}

void storage_off()
{
  if(s_backend==StorageBackend::SD_MMC) unmountSD_MMC();
}

// -----------------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------------
static fs::FS& activeFS()
{
  if(sdOK){
    if(s_backend==StorageBackend::SD_MMC) return SD_MMC;
    if(s_backend==StorageBackend::SD_SPI) return SD;
  }
  return LittleFS;
}

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

static bool mountSD_SPI()
{
  LOG_STORAGE("SD SPI","init");
  s_sdSPI.begin(SD_SPI_SCK,SD_SPI_MISO,SD_SPI_MOSI,SD_SPI_CS);
  if(!SD.begin(SD_SPI_CS,s_sdSPI,SD_SPI_FREQ_HZ)){
    LOG_ERROR("SD SPI","begin failed"); return false;
  }
  return true;
}

static bool quickIOTest(fs::FS& fs,const char* path)
{
  File f=fs.open(path,FILE_WRITE);
  if(!f) return false;
  f.println("ok"); f.flush(); f.close();
  fs.remove(path); return true;
}

static void sdBytes(uint64_t& total,uint64_t& used)
{
  total=used=0;
  if(s_backend==StorageBackend::SD_MMC){ total=SD_MMC.totalBytes(); used=SD_MMC.usedBytes(); }
  else if(s_backend==StorageBackend::SD_SPI){ total=SD.totalBytes(); used=SD.usedBytes(); }
}

static void logSDStats()
{
  uint64_t total=0,used=0; sdBytes(total,used);
  const uint64_t freeb=(total>used)?(total-used):0;
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
