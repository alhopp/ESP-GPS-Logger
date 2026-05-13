#pragma once

// ============================================================================
// geojson_session_export.h
//
// Session GeoJSON export API. Converts a closed SBP session and its stats
// snapshot into the public GeoJSON session file.
// ============================================================================

struct SessionStatsSnapshot;

bool geojson_session_export_finalize(const char* sbpPath, const char* geojsonPath);
bool geojson_session_export_finalize(const char* sbpPath,
                                     const char* geojsonPath,
                                     const SessionStatsSnapshot& snapshot);
