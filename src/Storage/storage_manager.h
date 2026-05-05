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
fs::FS& storage_sd_fs();
bool storage_logs_dir_ready();
bool storage_is_shutting_down();
void storage_begin_shutdown();
void storage_end_shutdown();
