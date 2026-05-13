#pragma once

// ============================================================================
// geojson_graph_writer.h
//
// Serializes GeoJSON graph properties for the base track feature. Keeps graph
// JSON formatting separate from the feature/document writer state machine.
// ============================================================================

#include <FS.h>

#include "Logging/GeoJSON/geojson_types.h"

void geojson_write_graphs(File& file, const GeoJSONGraphs& graphs);
