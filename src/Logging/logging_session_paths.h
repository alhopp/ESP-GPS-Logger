#pragma once

struct SessionPaths {
  char base[96];
  char ubx[128];
  char sbp[128];
  char geojson[128];
};

void logging_session_paths_build(SessionPaths& paths);
