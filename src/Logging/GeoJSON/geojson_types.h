#pragma once

// ============================================================================
// geojson_types.h
//
// Shared data shapes used to attach session statistics and graph series to the
// GeoJSON base track feature.
// ============================================================================

struct GeoJSONStats {
  float nm;
  float alpha;
  float alphaDistance;
  float alphaClosure;
  float h1;
  float max;
  float avg10;
  float r10[5];
  float distance;
};

struct GeoJSONGraphSeries {
  const float* values;
  int count;
  float xMax;
  const char* xUnit;
};

struct GeoJSONGraphs {
  GeoJSONGraphSeries s2;
  GeoJSONGraphSeries s10[5];
  int s10Count;
  GeoJSONGraphSeries alpha;
  GeoJSONGraphSeries nm;
  GeoJSONGraphSeries h1;
  GeoJSONGraphSeries distance;
};
