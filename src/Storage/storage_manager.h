#pragma once

#include <Arduino.h>

// ----------------------------------------------------
// Public API
// ----------------------------------------------------
void initStorage();
bool storage_on();
bool storage_off();

// ----------------------------------------------------
// Global storage state
// ----------------------------------------------------
extern bool sdOK;
extern bool littlefsOK;

// Hardware-fixed SD configuration
static constexpr const char* SD_MMC_MOUNTPOINT = "/sdcard";
static constexpr bool SD_MMC_1BIT_MODE = true;

extern bool storage_shutting_down;