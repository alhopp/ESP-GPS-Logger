#pragma once

// ============================================================================
// gps_runtime_state.h
//
// Shared GPS runtime status/state.
//
// These names are still intentionally global because the metric pipeline is
// legacy/RP6-compatible and several modules need to read the same live state.
// Keep GPS-owned state here instead of Core/Globals.
// ============================================================================

// Receiver/status state.
extern bool GPS_Signal_OK;
extern int  last_gps_msg;
extern int  nav_pvt_message;
extern int  nav_sat_message;

// Session/run state shared by the metric calculators and display.
extern int   run_count;
extern int   old_run_count;
extern int   gps_speed_value;
