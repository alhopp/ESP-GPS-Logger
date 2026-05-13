// ============================================================================
// geojson_writer.cpp
//
// Low-level streaming GeoJSON file writer. Manages GeoJSON document structure,
// point/line feature serialization, and coordinate formatting.
// ============================================================================

#include "Logging/GeoJSON/geojson_writer.h"
#include <string.h>

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

void writeGraphSeries(const char* name, const GeoJSONGraphSeries& series)
{
  state.file.print("\"");
  state.file.print(name);
  state.file.print("\":[");

  for (int i = 0; i < series.count; i++) {
    if (i) state.file.print(",");
    state.file.print(series.values[i], 2);
  }

  state.file.print("]");
}

void writeGraphSeriesList(const char* name, const GeoJSONGraphSeries* series, int count)
{
  state.file.print("\"");
  state.file.print(name);
  state.file.print("\":[");

  for (int i = 0; i < count; i++) {
    if (i) state.file.print(",");
    state.file.print("[");
    for (int j = 0; j < series[i].count; j++) {
      if (j) state.file.print(",");
      state.file.print(series[i].values[j], 2);
    }
    state.file.print("]");
  }

  state.file.print("]");
}

void writeGraphMetaSeries(const char* name, const GeoJSONGraphSeries& series)
{
  state.file.print("\"");
  state.file.print(name);
  state.file.print("\":{\"xMax\":");
  state.file.print(series.xMax, 3);
  state.file.print(",\"xUnit\":\"");
  state.file.print(series.xUnit ? series.xUnit : "");
  state.file.print("\"}");
}

void writeGraphs()
{
  state.file.println(",");
  state.file.println("\"graphs\":{");
  writeGraphSeries("2s", state.graphs.s2);
  state.file.println(",");
  writeGraphSeriesList("10s", state.graphs.s10, state.graphs.s10Count);
  state.file.println(",");
  writeGraphSeries("alpha", state.graphs.alpha);
  state.file.println(",");
  writeGraphSeries("nm", state.graphs.nm);
  state.file.println(",");
  writeGraphSeries("1h", state.graphs.h1);
  state.file.println(",");
  writeGraphSeries("distance", state.graphs.distance);
  state.file.println();
  state.file.print("},");

  state.file.println();
  state.file.println("\"graph_meta\":{");
  writeGraphMetaSeries("2s", state.graphs.s2);
  state.file.println(",");
  writeGraphMetaSeries("10s", state.graphs.s10Count > 0 ? state.graphs.s10[0] : state.graphs.s2);
  state.file.println(",");
  writeGraphMetaSeries("alpha", state.graphs.alpha);
  state.file.println(",");
  writeGraphMetaSeries("nm", state.graphs.nm);
  state.file.println(",");
  writeGraphMetaSeries("1h", state.graphs.h1);
  state.file.println(",");
  writeGraphMetaSeries("distance", state.graphs.distance);
  state.file.println();
  state.file.print("}");
}

bool isTrackFeature()
{
  return state.currentMode && strcmp(state.currentMode, "track") == 0;
}

void writeStats()
{
  state.file.println(",");
  state.file.println("\"stats\":{");
  state.file.print("\"nm\":");
  state.file.print(state.stats.nm, 3);
  state.file.println(",");
  state.file.print("\"alpha\":");
  state.file.print(state.stats.alpha, 3);
  state.file.println(",");
  state.file.print("\"alphaDistance\":");
  state.file.print(state.stats.alphaDistance, 1);
  state.file.println(",");
  state.file.print("\"alphaClosure\":");
  state.file.print(state.stats.alphaClosure, 1);
  state.file.println(",");
  state.file.print("\"h1\":");
  state.file.print(state.stats.h1, 3);
  state.file.println(",");
  state.file.print("\"max\":");
  state.file.print(state.stats.max, 3);
  state.file.println(",");
  state.file.print("\"avg10\":");
  state.file.print(state.stats.avg10, 3);
  state.file.println(",");
  state.file.print("\"r10\":[");
  for (int i = 0; i < 5; i++) {
    if (i) state.file.print(",");
    state.file.print(state.stats.r10[i], 3);
  }
  state.file.println("],");
  state.file.print("\"distance\":");
  state.file.print(state.stats.distance, 3);
  state.file.println("}");
}

void writeTrackProperties()
{
  if (state.hasStats) {
    writeStats();
  }

  if (state.hasGraphs) {
    writeGraphs();
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
