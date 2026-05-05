#pragma once

// -----------------------------------------------------------------------------
// GeoJSON session writer
// -----------------------------------------------------------------------------

bool geojson_begin(const char* filename);

// Feature lifecycle
void geojson_begin_feature(const char* mode);
void geojson_add_point(double lat, double lon);
void geojson_end_feature();

void geojson_end();

// -----------------------------------------------------------------------------
// Session statistics (attached to base track only)
// -----------------------------------------------------------------------------

struct GeoJSONStats {
  float nm;
  float alpha;
  float h1;
  float max;
  float avg10;
  float distance;
};

void geojson_set_stats(const GeoJSONStats& s);
