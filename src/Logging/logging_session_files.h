#pragma once

// ============================================================================
// logging_session_files.h
//
// Session file lifecycle API. Owns opening, writing to, and closing the active
// UBX/SBP log files plus final GeoJSON export handoff.
// ============================================================================

bool logging_session_files_open();
void logging_session_files_write_raw();
bool logging_session_files_close();
