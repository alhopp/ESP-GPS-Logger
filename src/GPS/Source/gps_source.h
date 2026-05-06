#pragma once

// ============================================================================
// gps_source.h
//
// Single read point for GPS messages.
//
// Runtime code calls this function whether the build is using the real u-blox
// parser or the compile-time simulator.
// ============================================================================

// Returns the next GPS parser/simulator message type.
int gps_source_next_message();
