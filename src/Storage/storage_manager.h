#pragma once

#include <FS.h>

// ============================================================================
// storage_manager.h
//
// Filesystem ownership boundary.
//
// LittleFS is the always-on control/config filesystem. SD_MMC is the removable
// high-volume logging filesystem. Callers should not mount/unmount SD_MMC
// directly; use this module so logging, web downloads, and mode transitions all
// share the same storage state.
// ============================================================================

// Mount LittleFS and try to detect/mount SD_MMC during boot.
void initStorage();

// Ensure SD_MMC is mounted before a caller uses storage_sd_fs().
bool storage_on();

// Cleanly unmount SD_MMC. Open log files must be closed by their owner first.
bool storage_off();

// Availability/status flags.
bool storage_sd_available();
bool storage_littlefs_available();
uint32_t storage_sd_total_mb();
uint32_t storage_sd_used_mb();
uint32_t storage_sd_free_mb();

// Returns the SD_MMC filesystem object. Call storage_on() or
// storage_logs_dir_ready() first unless the current system mode already did it.
fs::FS& storage_sd_fs();

// Ensure SD_MMC is mounted and /logs exists.
bool storage_logs_dir_ready();

// Shutdown guard used to stop new file writes while a mode transition is
// closing files and unmounting SD_MMC.
bool storage_is_shutting_down();
void storage_begin_shutdown();
void storage_end_shutdown();
