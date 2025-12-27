// -----------------------------------------------------------------------------
// main.h
//
// System entry-point interface.
//
// Exposes minimal global state required by other subsystems.
// All startup logic and task orchestration remain private to main.cpp.
// -----------------------------------------------------------------------------

#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// GLOBAL SYSTEM STATE
// -----------------------------------------------------------------------------

// Indicates whether the system is currently in sleep mode
extern bool sleep_mode;

