#include "Logging/geojson_writer.h"
#include <math.h>
#include <string.h>

#include "Storage/storage_manager.h"

// -----------------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------------
namespace {
File geoFile;

bool firstFeature = true;
bool firstPoint = true;

bool hasStats = false;
GeoJSONStats stats;
bool hasGraphs = false;
GeoJSONGraphs graphs;

const char* currentMode = nullptr;

struct TrackPoint {
  double lat;
  double lon;
};

bool adaptiveTrackActive = false;
bool haveTrackLast = false;
bool haveTrackPending = false;
TrackPoint trackLast;
TrackPoint trackPending;

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
  stats = s;
  hasStats = true;
}

void geojson_set_graphs(const GeoJSONGraphs& g)
{
  graphs = g;
  hasGraphs = true;
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
  adaptiveTrackActive = false;
  haveTrackLast = false;
  haveTrackPending = false;
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
  if (!geoFile || !currentMode) return;

  if (!firstPoint) {
    geoFile.println(",");
  }
  firstPoint = false;

  char buf[64];
  snprintf(buf, sizeof(buf),
           "[%.6f,%.6f]",
           lon, lat);

  geoFile.print(buf);
}

void writeTrackPoint(const TrackPoint& point)
{
  writeCoordinate(point.lat, point.lon);
  trackLast = point;
  haveTrackLast = true;
}

void writeGraphSeries(const char* name, const GeoJSONGraphSeries& series)
{
  geoFile.print("\"");
  geoFile.print(name);
  geoFile.print("\":[");

  for (int i = 0; i < series.count; i++) {
    if (i) geoFile.print(",");
    geoFile.print(series.values[i], 2);
  }

  geoFile.print("]");
}

void writeGraphs()
{
  geoFile.println(",");
  geoFile.println("\"graphs\":{");
  writeGraphSeries("2s", graphs.s2);
  geoFile.println(",");
  writeGraphSeries("10s", graphs.s10);
  geoFile.println(",");
  writeGraphSeries("alpha", graphs.alpha);
  geoFile.println(",");
  writeGraphSeries("nm", graphs.nm);
  geoFile.println(",");
  writeGraphSeries("1h", graphs.h1);
  geoFile.println(",");
  writeGraphSeries("distance", graphs.distance);
  geoFile.println();
  geoFile.print("}");
}

// -----------------------------------------------------------------------------
// Begin GeoJSON file
// -----------------------------------------------------------------------------
bool geojson_begin(const char* filename)
{
  if (geoFile) {
    geoFile.close();
  }

  fs::FS& storage = storage_sd_fs();
  if (storage.exists(filename)) {
    storage.remove(filename);
  }

  geoFile = storage.open(filename, FILE_WRITE);
  if (!geoFile) return false;

  firstFeature = true;
  currentMode = nullptr;
  hasStats = false;
  hasGraphs = false;

  geoFile.println("{");
  geoFile.println("\"type\":\"FeatureCollection\",");
  geoFile.println("\"features\":[");
  return true;
}

// -----------------------------------------------------------------------------
// Begin a new feature (track / 2s / 10s / alpha / nm / 1h)
// -----------------------------------------------------------------------------
void geojson_begin_feature(const char* mode)
{
  if (!geoFile) return;

  if (!firstFeature) {
    geoFile.println(",");
  }
  firstFeature = false;

  firstPoint = true;
  currentMode = mode;
  if (strcmp(mode, "track") == 0) {
    resetAdaptiveTrack();
    adaptiveTrackActive = true;
  }

  geoFile.println("{");
  geoFile.println("\"type\":\"Feature\",");
  geoFile.println("\"geometry\":{");
  geoFile.println("\"type\":\"LineString\",");
  geoFile.println("\"coordinates\":[");
}

// -----------------------------------------------------------------------------
// Append coordinate to current feature
// -----------------------------------------------------------------------------
void geojson_add_point(double lat, double lon)
{
  if (!geoFile || !currentMode) return;

  writeCoordinate(lat, lon);
}

void geojson_add_track_point(double lat, double lon)
{
  if (!geoFile || !currentMode) return;
  if (!adaptiveTrackActive || strcmp(currentMode, "track") != 0) {
    geojson_add_point(lat, lon);
    return;
  }
  if (!coordinateLooksValid(lat, lon)) return;

  TrackPoint point { lat, lon };

  if (!haveTrackLast) {
    writeTrackPoint(point);
    return;
  }

  if (!haveTrackPending) {
    if (distanceMeters(trackLast, point) < TRACK_MIN_POINT_M) return;
    trackPending = point;
    haveTrackPending = true;
    return;
  }

  const double lastToCurrentM = distanceMeters(trackLast, point);
  const double lastToPendingM = distanceMeters(trackLast, trackPending);
  const double turnErrorM = perpendicularErrorMeters(trackLast, point, trackPending);
  const double turnDeg = headingDeltaDeg(
    headingDeg(trackLast, trackPending),
    headingDeg(trackPending, point)
  );

  const bool forceDistance = lastToCurrentM >= TRACK_MAX_STRAIGHT_M;
  const bool keepTurn = lastToPendingM >= TRACK_MIN_POINT_M &&
                        (turnErrorM >= TRACK_TURN_ERROR_M || turnDeg >= TRACK_TURN_DEG);

  if (forceDistance || keepTurn) {
    writeTrackPoint(trackPending);
  }

  trackPending = point;
  haveTrackPending = true;
}


// -----------------------------------------------------------------------------
// End current feature
// -----------------------------------------------------------------------------
void geojson_end_feature()
{
  if (!geoFile || !currentMode) return;

  if (adaptiveTrackActive && strcmp(currentMode, "track") == 0 && haveTrackPending) {
    writeTrackPoint(trackPending);
  }
  resetAdaptiveTrack();

  geoFile.println();
  geoFile.println("]");     // end coordinates array
  geoFile.println("},");    // close geometry object

  geoFile.println("\"properties\":{");

  geoFile.print("\"mode\":\"");
  geoFile.print(currentMode);
  geoFile.print("\"");

  // Attach session stats ONLY to base track
  if (hasStats && strcmp(currentMode, "track") == 0) {
    geoFile.println(",");
    geoFile.println("\"stats\":{");
    geoFile.print("\"nm\":");
    geoFile.print(stats.nm, 3);
    geoFile.println(",");
    geoFile.print("\"alpha\":");
    geoFile.print(stats.alpha, 3);
    geoFile.println(",");
    geoFile.print("\"h1\":");
    geoFile.print(stats.h1, 3);
    geoFile.println(",");
    geoFile.print("\"max\":");
    geoFile.print(stats.max, 3);
    geoFile.println(",");
    geoFile.print("\"avg10\":");
    geoFile.print(stats.avg10, 3);
    geoFile.println(",");
    geoFile.print("\"distance\":");
    geoFile.print(stats.distance, 3);
    geoFile.println("}");
  }

  if (hasGraphs && strcmp(currentMode, "track") == 0) {
    writeGraphs();
  }

  geoFile.println("}");   // end properties
  geoFile.println("}");   // end feature

  currentMode = nullptr;
}

// -----------------------------------------------------------------------------
// Finalise GeoJSON file
// -----------------------------------------------------------------------------
void geojson_end()
{
  if (!geoFile) return;

  geoFile.println();
  geoFile.println("]");
  geoFile.println("}");

  geoFile.flush();
  geoFile.close();
}
