#pragma once

#include <stdint.h>

#include "GPS/gps_fix.h"

// Owns the runtime policy for starting a GPS logging session.
// The task loop decides when a fix has arrived; this module decides whether
// the session can begin yet and performs the existing time-sync gate.

void gps_logging_policy_note_signal_ready(uint32_t nowMs);

// Returns true once a logging session is active.
bool gps_logging_policy_maybe_start_session(const GpsFix& fix);
