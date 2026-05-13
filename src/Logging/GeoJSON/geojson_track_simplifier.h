#pragma once

// ============================================================================
// geojson_track_simplifier.h
//
// Adaptive track-point simplifier for GeoJSON export. Decides which incoming
// track coordinates should be emitted while preserving turns and long straights.
// ============================================================================

struct GeoJsonTrackPoint {
  double lat;
  double lon;
};

void geojson_track_simplifier_reset();
bool geojson_track_simplifier_add(double lat, double lon, GeoJsonTrackPoint& out);
bool geojson_track_simplifier_flush(GeoJsonTrackPoint& out);
