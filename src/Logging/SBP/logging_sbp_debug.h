#pragma once

// ============================================================================
// logging_sbp_debug.h
//
// Optional SBP debug sample printer API. Used after session close to inspect
// sample windows behind computed statistics when debug logging is enabled.
// ============================================================================

struct SessionStatsSnapshot;

void logging_sbp_debug_print_samples(const char* sbpPath, const SessionStatsSnapshot& snapshot);
