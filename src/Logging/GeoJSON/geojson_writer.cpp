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

} // namespace

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

namespace {

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

void writeCommaLine(File& file)
{
  file.println(",");
}

void writePropertyName(File& file, const char* name)
{
  file.print("\"");
  file.print(name);
  file.print("\":");
}

void writeStats(File& file, const GeoJSONStats& stats)
{
  writeCommaLine(file);
  file.println("\"stats\":{");
  writePropertyName(file, "nm");
  file.print(stats.nm, 3);
  writeCommaLine(file);
  writePropertyName(file, "alpha");
  file.print(stats.alpha, 3);
  writeCommaLine(file);
  writePropertyName(file, "alphaDistance");
  file.print(stats.alphaDistance, 1);
  writeCommaLine(file);
  writePropertyName(file, "alphaClosure");
  file.print(stats.alphaClosure, 1);
  writeCommaLine(file);
  writePropertyName(file, "h1");
  file.print(stats.h1, 3);
  writeCommaLine(file);
  writePropertyName(file, "max");
  file.print(stats.max, 3);
  writeCommaLine(file);
  writePropertyName(file, "avg10");
  file.print(stats.avg10, 3);
  writeCommaLine(file);
  writePropertyName(file, "r10");
  file.print("[");
  for (int i = 0; i < 5; i++) {
    if (i) file.print(",");
    file.print(stats.r10[i], 3);
  }
  file.println("],");
  writePropertyName(file, "distance");
  file.print(stats.distance, 3);
  file.println("}");
}

void writeGraphSeries(File& file, const char* name, const GeoJSONGraphSeries& series)
{
  writePropertyName(file, name);
  file.print("[");

  for (int i = 0; i < series.count; i++) {
    if (i) file.print(",");
    file.print(series.values[i], 2);
  }

  file.print("]");
}

void writeGraphSeriesList(File& file, const char* name, const GeoJSONGraphSeries* series, int count)
{
  writePropertyName(file, name);
  file.print("[");

  for (int i = 0; i < count; i++) {
    if (i) file.print(",");
    file.print("[");
    for (int j = 0; j < series[i].count; j++) {
      if (j) file.print(",");
      file.print(series[i].values[j], 2);
    }
    file.print("]");
  }

  file.print("]");
}

void writeGraphMetaSeries(File& file, const char* name, const GeoJSONGraphSeries& series)
{
  writePropertyName(file, name);
  file.print("{\"xMax\":");
  file.print(series.xMax, 3);
  file.print(",\"xUnit\":\"");
  file.print(series.xUnit ? series.xUnit : "");
  file.print("\"}");
}

void writeGraphs(File& file, const GeoJSONGraphs& graphs)
{
  writeCommaLine(file);
  file.println("\"graphs\":{");
  writeGraphSeries(file, "2s", graphs.s2);
  writeCommaLine(file);
  writeGraphSeriesList(file, "10s", graphs.s10, graphs.s10Count);
  writeCommaLine(file);
  writeGraphSeries(file, "alpha", graphs.alpha);
  writeCommaLine(file);
  writeGraphSeries(file, "nm", graphs.nm);
  writeCommaLine(file);
  writeGraphSeries(file, "1h", graphs.h1);
  writeCommaLine(file);
  writeGraphSeries(file, "distance", graphs.distance);
  file.println();
  file.print("},");

  file.println();
  file.println("\"graph_meta\":{");
  writeGraphMetaSeries(file, "2s", graphs.s2);
  writeCommaLine(file);
  writeGraphMetaSeries(file, "10s", graphs.s10Count > 0 ? graphs.s10[0] : graphs.s2);
  writeCommaLine(file);
  writeGraphMetaSeries(file, "alpha", graphs.alpha);
  writeCommaLine(file);
  writeGraphMetaSeries(file, "nm", graphs.nm);
  writeCommaLine(file);
  writeGraphMetaSeries(file, "1h", graphs.h1);
  writeCommaLine(file);
  writeGraphMetaSeries(file, "distance", graphs.distance);
  file.println();
  file.print("}");
}

void writeTrackProperties()
{
  if (state.hasStats) {
    writeStats(state.file, state.stats);
  }

  if (state.hasGraphs) {
    writeGraphs(state.file, state.graphs);
  }
}

} // namespace

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
