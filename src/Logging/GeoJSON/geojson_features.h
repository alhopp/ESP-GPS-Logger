#pragma once

// ============================================================================
// geojson_features.h
//
// Adds base track and derived result LineString features to the active GeoJSON
// writer from a completed SBP session.
// ============================================================================

struct SessionStatsSnapshot;

bool geojson_add_base_track_from_sbp(const char* sbpPath);
void geojson_add_derived_features(const char* sbpPath, const SessionStatsSnapshot& snapshot);
