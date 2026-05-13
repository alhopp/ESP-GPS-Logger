#pragma once

// ============================================================================
// geojson_graphs.h
//
// Builds compact speed graph series for the GeoJSON track feature from the
// completed SBP session and final session statistics snapshot.
// ============================================================================

struct SessionStatsSnapshot;

void geojson_attach_graph_series(const char* sbpPath, const SessionStatsSnapshot& snapshot);
