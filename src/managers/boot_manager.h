#pragma once

// -----------------------------------------------------------------------------
// boot_manager.h
//
// Early boot sequence:
// - Starts Serial (bounded wait)
// - Samples battery voltage
// - Resets timebase
// - Initializes e-paper display (early, deterministic)
//
// Returns a BootResult describing whether boot may continue.
// This module does NOT change system mode or make UI decisions.
// -----------------------------------------------------------------------------

enum BootResult : uint8_t {
  BOOT_OK = 0,
  BOOT_LOW_BATTERY,
  BOOT_AFTER_RESET
};

// Run early boot checks and initialise core boot-time hardware
BootResult initBoot();

// Optional: fetch a human-readable reason for the last non-OK boot result.
// Returns nullptr if BOOT_OK.
const char* bootFailReason();

