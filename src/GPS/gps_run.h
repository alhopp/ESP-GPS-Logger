#pragma once

// ============================================================================
// gps_run
//
// Run + jibe detection (AUTHORITATIVE)
//
// - Detects straight sailing
// - Detects standstill + delayed restart
// - Detects jibe via heading deviation
// - Maintains monotonically increasing run_id
//
// Units:
// - heading : degrees
// - speed   : knots
// ============================================================================

// Update run state (call once per GPS sample)
void gps_run_update(float heading_deg, float speed_kn);

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
