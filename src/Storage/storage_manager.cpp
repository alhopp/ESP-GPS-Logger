// -----------------------------------------------------------------------------
// storage_manager.cpp
// ESP32 storage manager:
// - LittleFS = always-on control/config storage
// - SD_MMC   = removable/high-volume data logging storage
// -----------------------------------------------------------------------------

#include "Storage/storage_manager.h"

#include <SD_MMC.h>
#include <LittleFS.h>

#include "Core/board_pins.h"
#include "Core/Definitions.h"

// Internal storage state
static bool littlefs_available=false;
static bool sd_detected=false;
static bool sd_mounted=false;
static bool shutting_down=false;

// Internal helpers
static void setLittleFsAvailable(bool available);
static void setSdAvailable(bool available);
static bool mountSD_MMC();
static void unmountSD_MMC();
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
  setSdAvailable(false);
  setLittleFsAvailable(false);
  storage_end_shutdown();

  // Mount LittleFS so config/control storage is always available.
  if(!LittleFS.begin(true)) LOG_ERROR("LittleFS","mount failed");
  else{
    setLittleFsAvailable(true);
    LOG_STORAGE("LittleFS","mounted");
    logLittleFSStats();
  }

  // Mount SD_MMC for main data logging if card is present/usable.
  if(mountSD_MMC()){
    setSdAvailable(true);
    LOG_STORAGE("SD","MMC mounted");
    logSDStats();

    // Quick write/remove check to catch bad cards or broken mount states.
    if(quickIOTest(storage_sd_fs(),"/.io_test")) LOG_STORAGE("SD I/O","OK");
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
  if(!storage_sd_available()) return false;
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

bool storage_sd_available()
{
  return sd_detected;
}

bool storage_littlefs_available()
{
  return littlefs_available;
}

bool storage_sd_mounted()
{
  return sd_mounted;
}

fs::FS& storage_sd_fs()
{
  return static_cast<fs::FS&>(SD_MMC);
}

const char* storage_sd_mount_path()
{
  return SD_MMC_MOUNTPOINT;
}

bool storage_logs_dir_ready()
{
  if (!storage_on()) return false;

  fs::FS& fs = storage_sd_fs();
  if (!fs.exists("/logs")) {
    return fs.mkdir("/logs");
  }

  return true;
}

bool storage_is_shutting_down()
{
  return shutting_down;
}

void storage_begin_shutdown()
{
  shutting_down = true;
}

void storage_end_shutdown()
{
  shutting_down = false;
}

static void setLittleFsAvailable(bool available)
{
  littlefs_available = available;
}

static void setSdAvailable(bool available)
{
  sd_detected = available;
}

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
// - logging sessions must already be closed by the session owner
// - end SD_MMC bus
// - short delay for stability before continuing
static void unmountSD_MMC()
{
  if(!sd_mounted) return;

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
