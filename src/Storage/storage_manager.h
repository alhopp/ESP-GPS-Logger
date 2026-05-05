#pragma once

#include <Arduino.h>
#include <FS.h>

// ----------------------------------------------------
// Public API
// ----------------------------------------------------
void initStorage();
bool storage_on();
bool storage_off();

bool storage_sd_available();
bool storage_littlefs_available();
bool storage_sd_mounted();
fs::FS& storage_sd_fs();
const char* storage_sd_mount_path();
bool storage_logs_dir_ready();
bool storage_is_shutting_down();
void storage_begin_shutdown();
void storage_end_shutdown();

// Hardware-fixed SD configuration
static constexpr const char* SD_MMC_MOUNTPOINT = "/sdcard";
static constexpr bool SD_MMC_1BIT_MODE = true;
