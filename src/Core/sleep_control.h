#pragma once

// ============================================================================
// Deep sleep control
//
// Owns magnet-wake detection and the final ESP32 deep-sleep entry path.
// Display code requests sleep only after drawing the final sleep screen.
// ============================================================================

bool sleep_woke_from_magnet();
void sleep_enter_from_magnet();
