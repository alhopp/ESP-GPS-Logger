// ============================================================================
// gps_alpha_speed.cpp
//
// Alpha 500 - Speedreader-compatible window scan
//
// Behaviour:
// - Scans all start/end sample pairs inside the retained GPS history
// - Sailed distance is summed from Doppler speed and must be <= 500 m
// - End point must return within the configured alpha radius
// - No jibe/run-shape heuristic is required for validity
// - Speedreader-style low-speed filter zeroes samples below 0.6 kn
// - Short alpha candidates need at least max(100 m, 2 x closure radius) path
// ============================================================================

#include "GPS/Metrics/gps_alpha_speed.h"
#include "GPS/Data/gps_data.h"
#include "GPS/gps_config.h"
#include "GPS/Metrics/gps_distance_speed.h"

#include "Core/Globals.h"
#include "Core/system_info.h"
#include "GPS/gps_runtime_state.h"

#include <Arduino.h>
#include <math.h>
#include <time.h>

// -----------------------------------------------------------------------------
// Geometry window export (Alpha 500)
// Consumed by storage / GeoJSON writer
// -----------------------------------------------------------------------------
int alpha_start = -1;
int alpha_end   = -1;
int alpha_sbp_start = -1;
int alpha_sbp_end   = -1;
float alpha_best_speed_mmps = 0.0f;
float alpha_best_closure_m = 0.0f;
int alpha_best_distance_m = 0;

// -----------------------------------------------------------------------------
// External shared state
// -----------------------------------------------------------------------------
extern int index_GPS;
extern int alfa_counter;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
namespace {
constexpr double SPEED_TIE_EPS_MMPS = 2.0;
constexpr double ALPHA_MIN_DISTANCE_M = 100.0;
constexpr double ALPHA_MIN_DISTANCE_RADIUS_FACTOR = 2.0;
constexpr double ALPHA_MAX_TIME_S = 194.0;
constexpr double ALPHA_CLOSURE_TOLERANCE_M = 0.2;
constexpr double SPEEDREADER_MIN_SPEED_KNOTS = 0.6;
constexpr int ALPHA_RESULT_COUNT = 10;

bool isMeaningfullyFaster(double candidate, double best)
{
  return candidate > best + SPEED_TIE_EPS_MMPS;
}

template <typename T>
void swapValue(T& a, T& b)
{
  T tmp = a;
  a = b;
  b = tmp;
}

bool windowsOverlap(int startA, int endA, int startB, int endB)
{
  return startA <= endB && startB <= endA;
}

double alphaSpeedContribution(int gpsIndex)
{
  const int idx = gpsIndex % BUFFER_SIZE;
  if (!_sampleGood[idx]) return 0.0;

  const uint16_t speedMmps = _gSpeed[idx];
  if ((double)speedMmps * MMPS_TO_KNOTS < SPEEDREADER_MIN_SPEED_KNOTS) {
    return 0.0;
  }

  return (double)speedMmps;
}

int sbpStartForGpsIndex(int gpsIndex)
{
  if (gpsIndex < 1) return -1;

  const int storedSbpIndex = _sbpIndex[gpsIndex % BUFFER_SIZE];
  if (storedSbpIndex > 2) return storedSbpIndex - 2;

  return gpsIndex > 2 ? gpsIndex - 2 : 1;
}
}

static inline double closure_dist2(int a, int b)
{
  const double lat0 = _lat[a];
  const double lon0 = _long[a];
  const double lat1 = _lat[b];
  const double lon1 = _long[b];

  const double dlat = lat1 - lat0;
  const double dlon = (lon1 - lon0) * cos(DEG2RAD * lat1);

  const double k = 111195.0; // meters / degree
  return (dlat*dlat + dlon*dlon) * k * k;
}

void Alfa_speed::clearResults()
{
  for(int i=0;i<ALPHA_RESULT_COUNT;i++){
    avg_speed[i]      = 0.0;
    real_distance[i]  = 0;
    time_hour[i]      = 0;
    time_min[i]       = 0;
    time_sec[i]       = 0;
    this_run[i]       = 0;
    message_nr[i]     = 0;
    alfa_distance[i]  = 0;
    result_start[i]   = -1;
    result_end[i]     = -1;
    result_sbp_start[i] = -1;
    result_sbp_end[i]   = -1;
  }
}

void Alfa_speed::sortResults()
{
  for(int i=0;i<ALPHA_RESULT_COUNT-1;i++){
    for(int j=i+1;j<ALPHA_RESULT_COUNT;j++){
      if(avg_speed[i] > avg_speed[j]){
        swapValue(avg_speed[i], avg_speed[j]);
        swapValue(real_distance[i], real_distance[j]);
        swapValue(time_hour[i], time_hour[j]);
        swapValue(time_min[i], time_min[j]);
        swapValue(time_sec[i], time_sec[j]);
        swapValue(this_run[i], this_run[j]);
        swapValue(message_nr[i], message_nr[j]);
        swapValue(alfa_distance[i], alfa_distance[j]);
        swapValue(result_start[i], result_start[j]);
        swapValue(result_end[i], result_end[j]);
        swapValue(result_sbp_start[i], result_sbp_start[j]);
        swapValue(result_sbp_end[i], result_sbp_end[j]);
      }
    }
  }
}

int Alfa_speed::ResultSbpStart(int slot) const
{
  return (slot >= 0 && slot < ALPHA_RESULT_COUNT) ? result_sbp_start[slot] : -1;
}

int Alfa_speed::ResultSbpEnd(int slot) const
{
  return (slot >= 0 && slot < ALPHA_RESULT_COUNT) ? result_sbp_end[slot] : -1;
}

void Alfa_speed::recordCandidate(double speedMmps,
                                 int startGpsIndex,
                                 int endGpsIndex,
                                 double closureDist2,
                                 double distanceScaled,
                                 int sampleRate)
{
  if(speedMmps <= 0.0 || endGpsIndex < startGpsIndex) return;

  // Speedreader reports one representative result for a cluster of overlapping
  // alpha windows. A faster overlapping candidate replaces the slower one.
  for(int i=0;i<ALPHA_RESULT_COUNT;i++){
    if(avg_speed[i] <= 0.0 || result_start[i] < 0) continue;
    if(!windowsOverlap(startGpsIndex, endGpsIndex, result_start[i], result_end[i])) continue;
    if(!isMeaningfullyFaster(speedMmps, avg_speed[i])) return;
  }

  for(int i=0;i<ALPHA_RESULT_COUNT;i++){
    if(avg_speed[i] <= 0.0 || result_start[i] < 0) continue;
    if(!windowsOverlap(startGpsIndex, endGpsIndex, result_start[i], result_end[i])) continue;

    avg_speed[i] = 0.0;
    real_distance[i] = 0;
    time_hour[i] = 0;
    time_min[i] = 0;
    time_sec[i] = 0;
    this_run[i] = 0;
    message_nr[i] = 0;
    alfa_distance[i] = 0;
    result_start[i] = -1;
    result_end[i] = -1;
    result_sbp_start[i] = -1;
    result_sbp_end[i] = -1;
  }

  int slot = -1;
  for(int i=0;i<ALPHA_RESULT_COUNT;i++){
    if(avg_speed[i] <= 0.0) {
      slot = i;
      break;
    }
  }

  if(slot < 0) {
    sortResults();
    if(!isMeaningfullyFaster(speedMmps, avg_speed[0])) return;
    slot = 0;
  }

  avg_speed[slot] = speedMmps;
  real_distance[slot] = (int)closureDist2;

  getLocalTime(&tmstruct,0);
  time_hour[slot] = tmstruct.tm_hour;
  time_min [slot] = tmstruct.tm_min;
  time_sec [slot] = tmstruct.tm_sec;

  this_run[slot] = alfa_counter;
  message_nr[slot] = nav_pvt_message;
  alfa_distance[slot] = (int)(distanceScaled / (double)sampleRate);
  result_start[slot] = startGpsIndex;
  result_end[slot] = endGpsIndex;
  const int candidateSbpStart = sbpStartForGpsIndex(startGpsIndex);
  const int candidateSbpEnd = sbpStartForGpsIndex(endGpsIndex);
  result_sbp_start[slot] = candidateSbpStart;
  result_sbp_end[slot] = candidateSbpEnd;

  sortResults();

  alfa_speed = speedMmps;
  alfa_speed_max = avg_speed[ALPHA_RESULT_COUNT - 1];
  display_max_speed = (float)alfa_speed_max;

  if(export_best && isMeaningfullyFaster(speedMmps, alpha_best_speed_mmps)) {
    alpha_best_speed_mmps = speedMmps;
    alpha_start = startGpsIndex;
    alpha_end   = endGpsIndex;
    alpha_sbp_start = candidateSbpStart;
    alpha_sbp_end   = candidateSbpEnd;
    alpha_best_closure_m = sqrt(closureDist2);
    alpha_best_distance_m = (int)(distanceScaled / (double)sampleRate / 1000.0);
  }
}

// ============================================================================
// Alfa_speed constructor
// ============================================================================
Alfa_speed::Alfa_speed(int alfa_radius, bool export_best)
{
  this->export_best = export_best;
  alfa_circle_square = (double)alfa_radius * (double)alfa_radius;

  alfa_speed        = 0.0;
  alfa_speed_max    = 0.0;
  display_max_speed = 0.0;

  clearResults();

  old_run_count = -1;
}

// -----------------------------------------------------------------------------
// Update_Alfa
// -----------------------------------------------------------------------------
float Alfa_speed::Update_Alfa(const GPS_distance_speed& M)
{
  const int exit  = index_GPS;
  const int sampleRate = systemInfo.sample_rate > 0 ? systemInfo.sample_rate : 1;
  const double maxDistanceScaled = (double)M.m_set_distance * 1000.0 * (double)sampleRate;
  const double radiusM = sqrt(alfa_circle_square);
  const double radiusDistanceM = radiusM * ALPHA_MIN_DISTANCE_RADIUS_FACTOR;
  const double minDistanceM =
      radiusDistanceM > ALPHA_MIN_DISTANCE_M ? radiusDistanceM : ALPHA_MIN_DISTANCE_M;
  const double minDistanceScaled = minDistanceM * 1000.0 * (double)sampleRate;
  const int maxTimeSamples = (int)(ALPHA_MAX_TIME_S * (double)sampleRate);

  if(exit > 0 && maxDistanceScaled > 0.0 && _sampleGood[exit % BUFFER_SIZE])
  {
    double distanceScaled = 0.0;
    const int maxBackSamples = min(min(BUFFER_SIZE - 1, BUFFER_ALFA - 2), maxTimeSamples);
    const int oldest = exit - maxBackSamples;
    const double allowedClosureM = radiusM + ALPHA_CLOSURE_TOLERANCE_M;
    const double allowedClosure2 = allowedClosureM * allowedClosureM;

    for(int entry = exit; entry >= oldest; entry--)
    {
      distanceScaled += alphaSpeedContribution(entry);
      if(distanceScaled > maxDistanceScaled) break;
      if(distanceScaled < minDistanceScaled) continue;
      if(entry >= exit || !_sampleGood[entry % BUFFER_SIZE]) continue;

      const int closureStart = entry > 0 ? entry - 1 : entry;
      const int entryA = closureStart % BUFFER_ALFA;
      const int exitA  = exit  % BUFFER_ALFA;
      const double d2 = closure_dist2(entryA, exitA);
      if(d2 > allowedClosure2) continue;

      const int samples = exit - entry + 1;
      if(samples <= 0 || samples >= BUFFER_ALFA) continue;

      const double speed = distanceScaled / (double)samples;
      recordCandidate(speed, entry, exit, d2, distanceScaled, sampleRate);
    }
  }

  old_run_count = run_count;
  alfa_speed_max = avg_speed[ALPHA_RESULT_COUNT - 1];
  display_max_speed = (float)alfa_speed_max;

  return alfa_speed_max;
}

// -----------------------------------------------------------------------------
// Reset
// -----------------------------------------------------------------------------
void Alfa_speed::Reset_stats()
{
  clearResults();
  alfa_speed     = 0.0;
  alfa_speed_max = 0.0;
  display_max_speed = 0.0f;
  old_run_count = -1;
  if(export_best) {
    alpha_best_speed_mmps = 0.0f;
    alpha_best_closure_m = 0.0f;
    alpha_best_distance_m = 0;
    alpha_start = -1;
    alpha_end = -1;
    alpha_sbp_start = -1;
    alpha_sbp_end = -1;
  }
}

// -----------------------------------------------------------------------------
// Finalise_Run (explicit flush if needed)
// -----------------------------------------------------------------------------
void Alfa_speed::Finalise_Run()
{
  sortResults();
  alfa_speed_max = avg_speed[ALPHA_RESULT_COUNT - 1];
  display_max_speed = (float)alfa_speed_max;
}
