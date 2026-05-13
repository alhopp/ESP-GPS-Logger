// ============================================================================
// geojson_writer.cpp
//
// Low-level streaming GeoJSON file writer. Manages GeoJSON document structure,
// point/line feature serialization, and coordinate formatting.
// ============================================================================

#include "Logging/GeoJSON/geojson_writer.h"
#include <string.h>

#include "Logging/GeoJSON/geojson_graph_writer.h"
#include "Logging/GeoJSON/geojson_stats_writer.h"
#include "Logging/GeoJSON/geojson_track_simplifier.h"
#include "Storage/storage_manager.h"

// -----------------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------------
namespace {
struct GeoJsonWriterState {
  File file;

  bool firstFeature = true;
  bool firstPoint = true;

  bool hasStats = false;
  GeoJSONStats stats;
  bool hasGraphs = false;
  GeoJSONGraphs graphs;

  const char* currentMode = nullptr;
};

GeoJsonWriterState state;
}

// -----------------------------------------------------------------------------
// Stats setter (called once per session)
// -----------------------------------------------------------------------------
void geojson_set_stats(const GeoJSONStats& s)
{
  state.stats = s;
  state.hasStats = true;
}

void geojson_set_graphs(const GeoJSONGraphs& g)
{
  state.graphs = g;
  state.hasGraphs = true;
}

void writeCoordinate(double lat, double lon)
{
  if (!state.file || !state.currentMode) return;

  if (!state.firstPoint) {
    state.file.println(",");
  }
  state.firstPoint = false;

  char buf[64];
  snprintf(buf, sizeof(buf),
           "[%.6f,%.6f]",
           lon, lat);

  state.file.print(buf);
}

void writeTrackPoint(const GeoJsonTrackPoint& point)
{
  writeCoordinate(point.lat, point.lon);
}

bool isTrackFeature()
{
  return state.currentMode && strcmp(state.currentMode, "track") == 0;
}

void writeTrackProperties()
{
  if (state.hasStats) {
    geojson_write_stats(state.file, state.stats);
  }

  if (state.hasGraphs) {
    geojson_write_graphs(state.file, state.graphs);
  }
}

// -----------------------------------------------------------------------------
// Begin GeoJSON file
// -----------------------------------------------------------------------------
bool geojson_begin(const char* filename)
{
  if (state.file) {
    state.file.close();
  }

  fs::FS& storage = storage_sd_fs();
  if (storage.exists(filename)) {
    storage.remove(filename);
  }

  state.file = storage.open(filename, FILE_WRITE);
  if (!state.file) return false;

  state.firstFeature = true;
  state.currentMode = nullptr;
  state.hasStats = false;
  state.hasGraphs = false;

  state.file.println("{");
  state.file.println("\"type\":\"FeatureCollection\",");
  state.file.println("\"features\":[");
  return true;
}

// -----------------------------------------------------------------------------
// Begin a new feature (track / 2s / 10s / alpha / nm / 1h)
// -----------------------------------------------------------------------------
void geojson_begin_feature(const char* mode)
{
  if (!state.file) return;

  if (!state.firstFeature) {
    state.file.println(",");
  }
  state.firstFeature = false;

  state.firstPoint = true;
  state.currentMode = mode;
  if (strcmp(mode, "track") == 0) {
    geojson_track_simplifier_reset();
  }

  state.file.println("{");
  state.file.println("\"type\":\"Feature\",");
  state.file.println("\"geometry\":{");
  state.file.println("\"type\":\"LineString\",");
  state.file.println("\"coordinates\":[");
}

// -----------------------------------------------------------------------------
// Append coordinate to current feature
// -----------------------------------------------------------------------------
void geojson_add_point(double lat, double lon)
{
  if (!state.file || !state.currentMode) return;

  writeCoordinate(lat, lon);
}

void geojson_add_track_point(double lat, double lon)
{
  if (!state.file || !state.currentMode) return;
  if (!isTrackFeature()) {
    geojson_add_point(lat, lon);
    return;
  }

  GeoJsonTrackPoint point;
  if (geojson_track_simplifier_add(lat, lon, point)) {
    writeTrackPoint(point);
  }
}


// -----------------------------------------------------------------------------
// End current feature
// -----------------------------------------------------------------------------
void geojson_end_feature()
{
  if (!state.file || !state.currentMode) return;

  if (isTrackFeature()) {
    GeoJsonTrackPoint point;
    if (geojson_track_simplifier_flush(point)) {
      writeTrackPoint(point);
    }
  }
  geojson_track_simplifier_reset();

  state.file.println();
  state.file.println("]");     // end coordinates array
  state.file.println("},");    // close geometry object

  state.file.println("\"properties\":{");

  state.file.print("\"mode\":\"");
  state.file.print(state.currentMode);
  state.file.print("\"");

  // Attach session stats ONLY to base track
  if (isTrackFeature()) {
    writeTrackProperties();
  }

  state.file.println("}");   // end properties
  state.file.println("}");   // end feature

  state.currentMode = nullptr;
}

// -----------------------------------------------------------------------------
// Finalise GeoJSON file
// -----------------------------------------------------------------------------
void geojson_end()
{
  if (!state.file) return;

  state.file.println();
  state.file.println("]");
  state.file.println("}");

  state.file.flush();
  state.file.close();
}
