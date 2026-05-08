#pragma once

// ============================================================================
// gps_run_detector
//
// Run + jibe detection (AUTHORITATIVE)
//
// - Matches RP6 New_run_detection()
// - Detects standstill + delayed restart
// - Detects jibe via heading deviation from 15s mean heading
// - Maintains monotonically increasing run_id
//
// Units:
// - heading : degrees
// - speed   : mm/s, normally speed_2s.avg_s
// ============================================================================

// Update run state (call once per GPS sample)
void gps_run_update(float heading_deg, float speed_mmps);

// Reset persistent detector state at the start of a new logging session.
void gps_run_reset();

// ---------------- lifecycle queries ----------------

// Current run number (starts at 0, first run becomes 1)
int  gps_run_current();

// True ONLY on the sample where a new run starts
bool gps_run_started();

// True ONLY on the sample where a run ends
bool gps_run_ended();

// ---------------- jibe info ----------------

// GPS sample index where last jibe occurred
int  gps_run_last_jibe_index();

// Diagnostic counters for RP6 parity checks.
int gps_run_armed_count();
int gps_run_jibe_count();
int gps_run_standstill_restart_count();
int gps_run_last_armed_index();
