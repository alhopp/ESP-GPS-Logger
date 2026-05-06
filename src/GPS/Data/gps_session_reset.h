#pragma once

// ============================================================================
// gps_session_reset.h
//
// One-shot reset for GPS-derived runtime state.
//
// Call this when starting a new logging/statistics session. Do not call it from
// normal sample flow, because it clears rolling windows, run detection, alpha
// state, duplicate-message guards, and session distance totals.
// ============================================================================

void reset_session_stats();
