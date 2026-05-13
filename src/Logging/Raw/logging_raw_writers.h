#pragma once

// ============================================================================
// logging_raw_writers.h
//
// Raw log writer API for the active session. Bridges current GNSS/parser state
// into UBX and SBP file writes owned by logging_session_files.
// ============================================================================

#include <FS.h>

void logging_raw_writers_reset();
void logging_raw_writers_write_ubx(File& ubxfile);
void logging_raw_writers_write_sbp(File& sbpfile);
