// ============================================================================
// geojson_track_simplifier.cpp
//
// Keeps the adaptive track simplification state and geometry calculations out
// of the low-level GeoJSON writer.
// ============================================================================

#include "Logging/GeoJSON/geojson_track_simplifier.h"

#include <math.h>

namespace {
struct SimplifierState {
  bool haveLast = false;
  bool havePending = false;
  GeoJsonTrackPoint last;
  GeoJsonTrackPoint pending;
};

SimplifierState state;

constexpr double TRACK_MIN_POINT_M = 4.0;
constexpr double TRACK_MAX_STRAIGHT_M = 80.0;
constexpr double TRACK_TURN_ERROR_M = 5.0;
constexpr double TRACK_TURN_DEG = 12.0;
constexpr double DEG_TO_RAD_LOCAL = 0.017453292519943295;
constexpr double RAD_TO_DEG_LOCAL = 57.29577951308232;

bool coordinateLooksValid(double lat, double lon)
{
  return isfinite(lat) && isfinite(lon) &&
         fabs(lat) <= 90.0 &&
         fabs(lon) <= 180.0 &&
         (fabs(lat) >= 0.001 || fabs(lon) >= 0.001);
}

double distanceMeters(const GeoJsonTrackPoint& a, const GeoJsonTrackPoint& b)
{
  const double lat = (a.lat + b.lat) * 0.5 * DEG_TO_RAD_LOCAL;
  const double dlat = b.lat - a.lat;
  const double dlon = (b.lon - a.lon) * cos(lat);
  return sqrt(dlat * dlat + dlon * dlon) * 111195.0;
}

double headingDeg(const GeoJsonTrackPoint& a, const GeoJsonTrackPoint& b)
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

double perpendicularErrorMeters(const GeoJsonTrackPoint& a,
                                const GeoJsonTrackPoint& b,
                                const GeoJsonTrackPoint& p)
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
}

void geojson_track_simplifier_reset()
{
  state.haveLast = false;
  state.havePending = false;
}

bool geojson_track_simplifier_add(double lat, double lon, GeoJsonTrackPoint& out)
{
  if (!coordinateLooksValid(lat, lon)) return false;

  const GeoJsonTrackPoint point { lat, lon };

  if (!state.haveLast) {
    state.last = point;
    state.haveLast = true;
    out = point;
    return true;
  }

  if (!state.havePending) {
    if (distanceMeters(state.last, point) < TRACK_MIN_POINT_M) return false;
    state.pending = point;
    state.havePending = true;
    return false;
  }

  const double lastToCurrentM = distanceMeters(state.last, point);
  const double lastToPendingM = distanceMeters(state.last, state.pending);
  const double turnErrorM = perpendicularErrorMeters(state.last, point, state.pending);
  const double turnDeg = headingDeltaDeg(
    headingDeg(state.last, state.pending),
    headingDeg(state.pending, point)
  );

  const bool forceDistance = lastToCurrentM >= TRACK_MAX_STRAIGHT_M;
  const bool keepTurn = lastToPendingM >= TRACK_MIN_POINT_M &&
                        (turnErrorM >= TRACK_TURN_ERROR_M || turnDeg >= TRACK_TURN_DEG);

  if (forceDistance || keepTurn) {
    out = state.pending;
    state.last = state.pending;
    state.pending = point;
    state.havePending = true;
    return true;
  }

  state.pending = point;
  state.havePending = true;
  return false;
}

bool geojson_track_simplifier_flush(GeoJsonTrackPoint& out)
{
  if (!state.havePending) return false;

  out = state.pending;
  state.last = state.pending;
  state.havePending = false;
  return true;
}
