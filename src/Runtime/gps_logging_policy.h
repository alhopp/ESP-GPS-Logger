#pragma once

// ============================================================================
// GPS logging policy
//
// Decides when a good GPS signal may become a logging session. The GPS task
// supplies fixes; this module owns the time-sync wait and session-begin retry
// gate before handing off to the logging subsystem.
// ============================================================================

#include <stdint.h>

#include "GPS/gps_fix.h"

void gps_logging_policy_note_signal_ready(uint32_t nowMs);

// Returns true once a logging session is active.
bool gps_logging_policy_maybe_start_session(const GpsFix& fix);
