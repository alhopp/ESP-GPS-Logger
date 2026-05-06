#pragma once

// Resets all GPS-derived session statistics and detector state.
// Use this at the start of a new logging session, not during normal sample flow.

void reset_session_stats();
