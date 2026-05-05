#pragma once

#include "GPS/gps_fix.h"

// Owns high-level logging session lifetime.
//
// Lower-level storage_file_operations still performs the actual UBX/SBP/GeoJSON
// writes, but callers should go through this API so session state has one owner.

bool logging_session_begin(const GpsFix& firstFix);
void logging_session_write_fix(const GpsFix& fix, bool writeLiveTrack);
void logging_session_end();
bool logging_session_active();
