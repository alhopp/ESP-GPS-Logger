// -----------------------------------------------------------------------------
// system_mode.h
//
// Central system mode state machine interface.
//
// Responsibilities:
// - Define the authoritative SystemMode enum
// - Expose read-only access to the current mode
// - Provide the ONLY legal mechanism for mode transitions
//
// Design rules:
// - All mode-related side-effects are owned by system_mode.cpp
//   (Wi-Fi, GPS power, sleep, etc.)
// - Other modules must NEVER perform mode-specific side-effects
// - UI and display logic must react to getMode(), not infer state
// -----------------------------------------------------------------------------

#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// SYSTEM MODES
// -----------------------------------------------------------------------------
enum SystemMode : uint8_t {
  MODE_BOOT = 0,        // Transitional startup state
  MODE_WAIT_SATS,       // Wait for Sats
  MODE_LOGGING,         // Primary mission: GPS logging, Wi-Fi OFF
  MODE_FIELD_CONFIG,    // User configuration: Wi-Fi AP, GPS OFF
  MODE_SLEEP            // Deep sleep: lowest power state
};

// -----------------------------------------------------------------------------
// PUBLIC API
// -----------------------------------------------------------------------------

// Return the current authoritative system mode
SystemMode getMode();

// Request a system mode transition
//
// Behaviour:
// - If newMode equals the current mode, the call is a no-op
// - EXIT → TRANSITION → ENTER side-effects are handled internally
// - Callers must not assume immediate completion of hardware changes
//
void setMode(SystemMode newMode);

// Optional helper for logging / diagnostics
// (Must NOT be used for UI or control logic)
const char* modeToString(SystemMode mode);
