#pragma once

// ============================================================================
// logging_session.h
//
// High-level logging session state API. Starts, stops, and reports whether a
// session is active while hiding file-level details from GPS processing code.
// ============================================================================

#include "GPS/gps_fix.h"

// Owns high-level logging session lifetime.
//
// Lower-level logging_session_files still performs the actual UBX/SBP/GeoJSON
// writes, but callers should go through this API so session state has one owner.

bool logging_session_begin(const GpsFix& firstFix);
void logging_session_write_fix();
void logging_session_end();
bool logging_session_active();
