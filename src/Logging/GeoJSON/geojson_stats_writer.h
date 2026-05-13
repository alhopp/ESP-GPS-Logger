#pragma once

// ============================================================================
// geojson_stats_writer.h
//
// Serializes GeoJSON session statistics for the base track feature.
// ============================================================================

#include <FS.h>

#include "Logging/GeoJSON/geojson_writer.h"

void geojson_write_stats(File& file, const GeoJSONStats& stats);
