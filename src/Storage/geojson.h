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

struct GeoJSONStats {
  float nm;
  float alpha;
  float h1;
  float max;
  float avg10;
  float distance;
};

void geojson_set_stats(const GeoJSONStats& s);

