#pragma once

// ============================================================================
// gps_manager.h
//
// GPS hardware bring-up orchestration for ESP32 + u-blox.
//
// This module owns startup sequencing only: power the receiver, find a working
// baud rate, apply u-blox configuration, and optionally inject RTC time for a
// warm start. Runtime parsing and statistics live in separate GPS modules.
// ============================================================================

enum class GpsLifecycleState {
  Off,
  Starting,
  Ready,
  Failed
};

// Bring up GPS.
//
// Returns:
// - true  = GPS responded and is alive
// - false = no GPS detected
bool initGPS();

// Power down GPS and mark the lifecycle state as Off.
void gps_shutdown();

// Current lifecycle state for diagnostics and UI/status reporting.
const char* gps_lifecycle_state_name();
