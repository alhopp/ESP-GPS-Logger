#pragma once

struct SessionStatsSnapshot;

bool geojson_session_export_finalize(const char* sbpPath, const char* geojsonPath);
bool geojson_session_export_finalize(const char* sbpPath,
                                     const char* geojsonPath,
                                     const SessionStatsSnapshot& snapshot);
