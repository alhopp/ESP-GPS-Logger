#pragma once

#include <stdint.h>

// ============================================================================
// GPS Manager API
// ============================================================================

// Bring up GPS:
// - Uses RTC cached baud if available
// - Falls back to baud scan
// - Injects RTC time for warm start
// Returns true if GPS responds
bool initGPS();

// Graceful shutdown (power off)
void gps_shutdown();

// Legacy compatibility (used elsewhere)
void Ublox_on();
void Ublox_off();
void gps_power_off();

