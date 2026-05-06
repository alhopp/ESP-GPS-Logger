// ============================================================================
// storage_manager.cpp
//
// ESP32 filesystem manager:
// - LittleFS = always-on control/config storage
// - SD_MMC   = removable/high-volume logging storage
//
// This module owns mount/unmount state. Other modules can open files, but they
// should not call LittleFS.begin(), SD_MMC.begin(), or SD_MMC.end() directly.
// ============================================================================

#include "Storage/storage_manager.h"

#include <LittleFS.h>
#include <SD_MMC.h>

#include "Core/board_pins.h"
#include "Core/log.h"

namespace {

// Hardware-fixed SD configuration. 1-bit mode is safer for this board wiring
// and avoids the wider SD bus pins.
constexpr const char* SD_MMC_MOUNTPOINT = "/sdcard";
constexpr bool SD_MMC_1BIT_MODE = true;

// Internal storage state. sd_detected records whether the card/bus has ever
// passed mount during this boot; sd_mounted records current bus state.
bool littlefs_available = false;
bool sd_detected = false;
bool sd_mounted = false;
bool shutting_down = false;

bool mountSD_MMC();
void unmountSD_MMC();
bool quickIOTest(fs::FS& fs, const char* path);
void logLittleFSStats();
void logSDStats();
void logDirectory(fs::FS& fs, const char* path, int depth = 0);

}

void initStorage()
{
  LOG_STORAGE("Init", "start");
  sd_detected = false;
  littlefs_available = false;
  storage_end_shutdown();

  // Mount LittleFS so config/control storage is always available.
  if (!LittleFS.begin(true)) {
    LOG_ERROR("LittleFS", "mount failed");
  } else {
    littlefs_available = true;
    LOG_STORAGE("LittleFS", "mounted");
    logLittleFSStats();
  }

  // Mount SD_MMC for main data logging if card is present/usable.
  if (mountSD_MMC()) {
    sd_detected = true;
    LOG_STORAGE("SD", "MMC mounted");
    logSDStats();

    // Quick write/remove check to catch bad cards or broken mount states.
    if (quickIOTest(storage_sd_fs(), "/.io_test")) {
      LOG_STORAGE("SD I/O", "OK");
    } else {
      LOG_ERROR("SD I/O", "FAILED");
    }

    logDirectory(storage_sd_fs(), "/logs");
  } else {
    LOG_STORAGE("SD", "not available");
  }

  LOG_STORAGE("Init", "done");
}

// Ensure SD_MMC is mounted before use.
// Returns false if SD was never successfully detected or remount fails.
bool storage_on()
{
  if (!storage_sd_available()) return false;
  return mountSD_MMC();
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

uint32_t storage_sd_total_mb()
{
  if (!sd_detected) return 0;
  return static_cast<uint32_t>(SD_MMC.totalBytes() / (1024ULL * 1024ULL));
}

uint32_t storage_sd_used_mb()
{
  if (!sd_detected) return 0;
  return static_cast<uint32_t>(SD_MMC.usedBytes() / (1024ULL * 1024ULL));
}

uint32_t storage_sd_free_mb()
{
  if (!sd_detected) return 0;

  const uint64_t total = SD_MMC.totalBytes();
  const uint64_t used = SD_MMC.usedBytes();
  const uint64_t freeb = total > used ? total - used : 0;
  return static_cast<uint32_t>(freeb / (1024ULL * 1024ULL));
}

fs::FS& storage_sd_fs()
{
  return static_cast<fs::FS&>(SD_MMC);
}

bool storage_logs_dir_ready()
{
  if (!storage_on()) return false;

  fs::FS& fs = storage_sd_fs();
  if (fs.exists("/logs")) return true;

  if (!fs.mkdir("/logs")) {
    LOG_ERROR("STORAGE", "mkdir /logs failed");
    return false;
  }

  return fs.exists("/logs");
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

namespace {

// Mount SD_MMC in 1-bit safe mode. DAT0 gets a pull-up preflight to improve
// bring-up reliability when the card/eMMC is not already driving the line.
bool mountSD_MMC()
{
  if (sd_mounted) return true;

  LOG_STORAGE("SD MMC", "preflight");
  pinMode(SDMMC_DAT0_PIN, INPUT_PULLUP);
  delay(2);

  LOG_STORAGE("SD MMC", "init");
  if (!SD_MMC.begin(SD_MMC_MOUNTPOINT, SD_MMC_1BIT_MODE)) {
    LOG_STORAGE("SD MMC", "no card");
    sd_mounted = false;
    return false;
  }

  sd_mounted = true;
  sd_detected = true;
  return true;
}

// Clean shutdown of SD_MMC:
// - logging sessions must already be closed by the session owner
// - end SD_MMC bus
// - short delay for stability before continuing
void unmountSD_MMC()
{
  if (!sd_mounted) return;

  LOG_STORAGE("SD MMC", "unmount");
  SD_MMC.end();
  vTaskDelay(pdMS_TO_TICKS(300));
  sd_mounted = false;
}

// Simple write/flush/remove test to validate filesystem usability.
bool quickIOTest(fs::FS& fs, const char* path)
{
  File f = fs.open(path, FILE_WRITE);
  if (!f) return false;

  f.println("ok");
  f.flush();
  f.close();
  return fs.remove(path);
}

// Log SD card capacity/usage in MB.
void logSDStats()
{
  const uint64_t total = SD_MMC.totalBytes();
  const uint64_t used = SD_MMC.usedBytes();
  const uint64_t freeb = (total > used) ? (total - used) : 0;

  LOG_STORAGE("SD Total", "%llu MB", (unsigned long long)(total / (1024ULL * 1024ULL)));
  LOG_STORAGE("SD Used", "%llu MB", (unsigned long long)(used / (1024ULL * 1024ULL)));
  LOG_STORAGE("SD Free", "%llu MB", (unsigned long long)(freeb / (1024ULL * 1024ULL)));
}

// Log LittleFS capacity/usage in KB.
void logLittleFSStats()
{
  LOG_STORAGE("LittleFS", "total=%uKB used=%uKB free=%uKB",
    (unsigned)(LittleFS.totalBytes() / 1024),
    (unsigned)(LittleFS.usedBytes() / 1024),
    (unsigned)((LittleFS.totalBytes() - LittleFS.usedBytes()) / 1024));
}

void logDirectory(fs::FS& fs, const char* path, int depth)
{
#if LOG_ENABLED
  File root = fs.open(path);
  if (!root) {
    LOG_STORAGE("SD Files", "%s not found", path);
    return;
  }

  if (!root.isDirectory()) {
    LOG_STORAGE("SD Files", "%s is not a directory", path);
    root.close();
    return;
  }

  LOG_STORAGE("SD Files", "%s", path);

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      LOG_STORAGE("SD Dir", "%s/", file.name());
      if (depth > 0) {
        logDirectory(fs, file.path(), depth - 1);
      }
    } else {
      LOG_STORAGE("SD File", "%s %llu bytes",
                  file.name(),
                  (unsigned long long)file.size());
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();
#else
  (void)fs;
  (void)path;
  (void)depth;
#endif
}

}
