// ============================================================================
// geojson_writer.cpp
//
// Low-level streaming GeoJSON file writer. Manages GeoJSON document structure,
// point/line feature serialization, and coordinate formatting.
// ============================================================================

#include "Logging/GeoJSON/geojson_writer.h"
#include <math.h>
#include <string.h>

#include "Storage/storage_manager.h"

// -----------------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------------
namespace {
struct TrackPoint {
  double lat;
  double lon;
};

struct GeoJsonWriterState {
  File file;

  bool firstFeature = true;
  bool firstPoint = true;

  bool hasStats = false;
  GeoJSONStats stats;
  bool hasGraphs = false;
  GeoJSONGraphs graphs;

  const char* currentMode = nullptr;

  bool adaptiveTrackActive = false;
  bool haveTrackLast = false;
  bool haveTrackPending = false;
  TrackPoint trackLast;
  TrackPoint trackPending;
};

GeoJsonWriterState state;

constexpr double TRACK_MIN_POINT_M = 4.0;
constexpr double TRACK_MAX_STRAIGHT_M = 80.0;
constexpr double TRACK_TURN_ERROR_M = 5.0;
constexpr double TRACK_TURN_DEG = 12.0;
constexpr double DEG_TO_RAD_LOCAL = 0.017453292519943295;
constexpr double RAD_TO_DEG_LOCAL = 57.29577951308232;
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

double distanceMeters(const TrackPoint& a, const TrackPoint& b)
{
  const double lat = (a.lat + b.lat) * 0.5 * DEG_TO_RAD_LOCAL;
  const double dlat = b.lat - a.lat;
  const double dlon = (b.lon - a.lon) * cos(lat);
  return sqrt(dlat * dlat + dlon * dlon) * 111195.0;
}

double headingDeg(const TrackPoint& a, const TrackPoint& b)
{
  const double lat = (a.lat + b.lat) * 0.5 * DEG_TO_RAD_LOCAL;
  const double x = (b.lon - a.lon) * cos(lat);
  const double y = b.lat - a.lat;
  double deg = atan2(x, y) * RAD_TO_DEG_LOCAL;
  if (deg < 0.0) deg += 360.0;
  return deg;
}

double headingDeltaDeg(double a, double b)
{
  double d = fabs(a - b);
  return d > 180.0 ? 360.0 - d : d;
}

double perpendicularErrorMeters(const TrackPoint& a,
                                const TrackPoint& b,
  const TrackPoint& p)
{
  const double lat = (a.lat + b.lat) * 0.5 * DEG_TO_RAD_LOCAL;
  const double ax = 0.0;
  const double ay = 0.0;
  const double bx = (b.lon - a.lon) * cos(lat) * 111195.0;
  const double by = (b.lat - a.lat) * 111195.0;
  const double px = (p.lon - a.lon) * cos(lat) * 111195.0;
  const double py = (p.lat - a.lat) * 111195.0;

  const double dx = bx - ax;
  const double dy = by - ay;
  const double len2 = dx * dx + dy * dy;
  if (len2 <= 0.001) return sqrt(px * px + py * py);

  double t = (px * dx + py * dy) / len2;
  if (t < 0.0) t = 0.0;
  if (t > 1.0) t = 1.0;

  const double ex = px - t * dx;
  const double ey = py - t * dy;
  return sqrt(ex * ex + ey * ey);
}

void resetAdaptiveTrack()
{
  state.adaptiveTrackActive = false;
  state.haveTrackLast = false;
  state.haveTrackPending = false;
}

bool coordinateLooksValid(double lat, double lon)
{
  return isfinite(lat) && isfinite(lon) &&
         fabs(lat) <= 90.0 &&
         fabs(lon) <= 180.0 &&
         (fabs(lat) >= 0.001 || fabs(lon) >= 0.001);
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

void writeTrackPoint(const TrackPoint& point)
{
  writeCoordinate(point.lat, point.lon);
  state.trackLast = point;
  state.haveTrackLast = true;
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
    resetAdaptiveTrack();
    state.adaptiveTrackActive = true;
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
  if (!state.adaptiveTrackActive || strcmp(state.currentMode, "track") != 0) {
    geojson_add_point(lat, lon);
    return;
  }
  if (!coordinateLooksValid(lat, lon)) return;

  TrackPoint point { lat, lon };

  if (!state.haveTrackLast) {
    writeTrackPoint(point);
    return;
  }

  if (!state.haveTrackPending) {
    if (distanceMeters(state.trackLast, point) < TRACK_MIN_POINT_M) return;
    state.trackPending = point;
    state.haveTrackPending = true;
    return;
  }

  const double lastToCurrentM = distanceMeters(state.trackLast, point);
  const double lastToPendingM = distanceMeters(state.trackLast, state.trackPending);
  const double turnErrorM = perpendicularErrorMeters(state.trackLast, point, state.trackPending);
  const double turnDeg = headingDeltaDeg(
    headingDeg(state.trackLast, state.trackPending),
    headingDeg(state.trackPending, point)
  );

  const bool forceDistance = lastToCurrentM >= TRACK_MAX_STRAIGHT_M;
  const bool keepTurn = lastToPendingM >= TRACK_MIN_POINT_M &&
                        (turnErrorM >= TRACK_TURN_ERROR_M || turnDeg >= TRACK_TURN_DEG);

  if (forceDistance || keepTurn) {
    writeTrackPoint(state.trackPending);
  }

  state.trackPending = point;
  state.haveTrackPending = true;
}


// -----------------------------------------------------------------------------
// End current feature
// -----------------------------------------------------------------------------
void geojson_end_feature()
{
  if (!state.file || !state.currentMode) return;

  if (state.adaptiveTrackActive &&
      strcmp(state.currentMode, "track") == 0 &&
      state.haveTrackPending) {
    writeTrackPoint(state.trackPending);
  }
  resetAdaptiveTrack();

  state.file.println();
  state.file.println("]");     // end coordinates array
  state.file.println("},");    // close geometry object

  state.file.println("\"properties\":{");

  state.file.print("\"mode\":\"");
  state.file.print(state.currentMode);
  state.file.print("\"");

  // Attach session stats ONLY to base track
  if (state.hasStats && strcmp(state.currentMode, "track") == 0) {
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

  if (state.hasGraphs && strcmp(state.currentMode, "track") == 0) {
    writeGraphs();
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
