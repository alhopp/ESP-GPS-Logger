#pragma once

// ============================================================================
// gps_manager.h
//
// GPS hardware bring-up orchestration for ESP32 + u-blox.
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

