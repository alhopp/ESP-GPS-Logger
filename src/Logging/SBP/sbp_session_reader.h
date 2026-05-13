#pragma once

// ============================================================================
// sbp_session_reader.h
//
// Shared reader API for completed SBP session logs. Keeps SBP file opening,
// frame seeking, and frame counting next to the canonical binary format.
// ============================================================================

#include <FS.h>

#include "Logging/SBP/sbp_format.h"

bool sbp_session_open(File& file, const char* sbpPath);
bool sbp_session_read_frame(File& file, SbpFrame& frame);
bool sbp_session_seek_frame(File& file, int sbpIndex);
bool sbp_session_read_frame_at(File& file, int sbpIndex, SbpFrame& frame);
int sbp_session_count_frames(const char* sbpPath);
