#pragma once

#include <Arduino.h>

// ----------------------------------------------------
// Public API
// ----------------------------------------------------
void initStorage();

// ----------------------------------------------------
// Global storage state
// ----------------------------------------------------
extern bool sdOK;
extern bool LITTLEFS_OK;
