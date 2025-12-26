// -----------------------------------------------------------------------------
// system_mode.h
//
// Central system mode state machine interface.
//
// Exposes:
// - SystemMode enum
// - Functions to query and change the current system mode
//
// The implementation owns all side-effects associated with mode transitions
// (Wi-Fi, power, etc.). Other modules should request mode changes only via
// setMode() and must not perform mode-specific side-effects themselves.
// -----------------------------------------------------------------------------

#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// SYSTEM MODES
// -----------------------------------------------------------------------------
enum SystemMode {
  MODE_BOOT = 0,
  MODE_LOGGING,
  MODE_FIELD_CONFIG,
  MODE_HOME,
  MODE_SLEEP
};

// -----------------------------------------------------------------------------
// PUBLIC API
// -----------------------------------------------------------------------------

// Return the current system mode
SystemMode getMode();

// Request a system mode transition
// NOTE:
// - If newMode equals the current mode, the call is a no-op
// - EXIT and ENTER side-effects are handled internally
void setMode(SystemMode newMode);
