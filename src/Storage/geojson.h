#pragma once

#include <Arduino.h>
#include <FS.h>

// -----------------------------------------------------------------------------
// GeoJSON session writer
//
// Responsibilities:
// - Create one GeoJSON FeatureCollection per session
// - Append GPS points incrementally
// - Finalise valid JSON on close
//
// Notes:
// - Coordinates are written as [lon, lat]
// - No dynamic allocation
// - SD write-safe
// -----------------------------------------------------------------------------

void geojson_begin(const char* filename);
void geojson_add_point(double lat, double lon);
void geojson_end();
