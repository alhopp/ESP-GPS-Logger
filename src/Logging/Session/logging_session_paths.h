#pragma once

// ============================================================================
// logging_session_paths.h
//
// Session filename/path builder API. Generates the base session name and the
// corresponding UBX, SBP, and GeoJSON paths under /logs.
// ============================================================================

struct SessionPaths {
  char base[96];
  char ubx[128];
  char sbp[128];
  char geojson[128];
};

void logging_session_paths_build(SessionPaths& paths);
