#pragma once

// ============================================================================
// gps_manager.h
//
// GPS hardware bring-up and power control for ESP32 + u-blox.
//
// -----------------------------------------------------------------------------
// PRIMARY API
// -----------------------------------------------------------------------------

// Bring up GPS:
// - Uses RTC-cached baud if available
// - Falls back to full baud scan
// - Injects RTC time for warm start
//
// Returns:
// - true  → GPS responded and is alive
// - false → no GPS detected
bool initGPS();


// -----------------------------------------------------------------------------
// SYSTEM POWER CONTROL
// -----------------------------------------------------------------------------
// These are low-level power primitives.
// Valid to call from system_mode or boot logic.
// Must be idempotent.

void gps_power_on();
void gps_power_off();



