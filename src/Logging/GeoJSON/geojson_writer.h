#pragma once

// ============================================================================
// geojson_writer.h
//
// Streaming GeoJSON writer API. Provides low-level helpers for opening a
// GeoJSON file and appending feature/geometry fragments safely.
// ============================================================================

#include "Logging/GeoJSON/geojson_types.h"

bool geojson_begin(const char* filename);

// Feature lifecycle
void geojson_begin_feature(const char* mode);
void geojson_add_point(double lat, double lon);
void geojson_add_track_point(double lat, double lon);
void geojson_end_feature();

void geojson_end();

// -----------------------------------------------------------------------------
// Session statistics (attached to base track only)
// -----------------------------------------------------------------------------

void geojson_set_stats(const GeoJSONStats& s);

void geojson_set_graphs(const GeoJSONGraphs& graphs);
