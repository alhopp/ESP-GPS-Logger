#pragma once

#include <Arduino.h>

// ----------------------------------------------------
// Public API
// ----------------------------------------------------
void initStorage();
uint64_t storageFreeKBytes();
int storageLogTimeLeftMinutes();

// ----------------------------------------------------
// Global storage state
// ----------------------------------------------------
extern bool sdOK;
extern bool LITTLEFS_OK;
