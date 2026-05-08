// ============================================================================
// gps_alpha_speed.cpp
//
// Alpha 500 — LEGACY-COMPATIBLE IMPLEMENTATION (speed_500m-driven)
//
// Behaviour (matches legacy Speedreader logic):
// - Uses distance-based GPS_distance_speed integrator for 500m window
// - Entry = (M.m_index + 1)
// - Exit  = current index_GPS
// - Closure < alfa_radius (50 m)
// - Speed = M.m_speed_alfa (SBP parity / distance-window exact)
// - Alpha is FINALISED on run change
// ============================================================================

#include "GPS/Metrics/gps_alpha_speed.h"
#include "GPS/Data/gps_data.h"
#include "GPS/Metrics/gps_distance_speed.h"
#include "GPS/Metrics/gps_result_sort.h"

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
constexpr double SPEED_TIE_EPS_MMPS = 1.0;

bool isMeaningfullyFaster(double candidate, double best)
{
  return candidate > best + SPEED_TIE_EPS_MMPS;
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

// ============================================================================
// Alfa_speed constructor
// ============================================================================
Alfa_speed::Alfa_speed(int alfa_radius)
{
  alfa_circle_square = (double)alfa_radius * (double)alfa_radius;

  alfa_speed        = 0.0;
  alfa_speed_max    = 0.0;
  display_max_speed = 0.0;

  for(int i=0;i<10;i++){
    avg_speed[i]      = 0.0;
    real_distance[i]  = 0;
    time_hour[i]      = 0;
    time_min[i]       = 0;
    time_sec[i]       = 0;
    this_run[i]       = 0;
    message_nr[i]     = 0;
    alfa_distance[i]  = 0;
  }

  old_run_count = -1;
}

// -----------------------------------------------------------------------------
// Update_Alfa (LEGACY / Speedreader-aligned)
// -----------------------------------------------------------------------------
float Alfa_speed::Update_Alfa(const GPS_distance_speed& M)
{
  const int exit  = index_GPS;
  const int entry = M.m_index + 1;

  // ---------------------------------------------------------------------------
  // Geometry + speed eligibility
  // ---------------------------------------------------------------------------
  if(entry >= 0 &&
     exit > entry &&
     M.m_speed_alfa > 0.0 &&
     _sampleGood[entry % BUFFER_SIZE] &&
     _sampleGood[exit % BUFFER_SIZE])
  {
    const int entryA = entry % BUFFER_ALFA;
    const int exitA  = exit  % BUFFER_ALFA;

    const double d2 = closure_dist2(entryA, exitA);

    if(d2 < alfa_circle_square && M.m_sample < BUFFER_ALFA)
    {
      const float speed = (float)M.m_speed_alfa;

      if(isMeaningfullyFaster(speed, alfa_speed_max))
      {
        alfa_speed_max = speed;
        alfa_speed     = speed;

        // Capture the session-best alpha geometry for GeoJSON/map export.
        // alfa_speed_max resets each run, so using it alone would let a later
        // slower run overwrite the overlay while the final stat still shows the
        // true best alpha from avg_speed[].
        if (isMeaningfullyFaster(speed, alpha_best_speed_mmps)) {
          alpha_best_speed_mmps = speed;
          alpha_start = entry;
          alpha_end   = exit;
          alpha_sbp_start = sbpStartForGpsIndex(entry);
          alpha_sbp_end   = sbpStartForGpsIndex(exit);
          alpha_best_closure_m = sqrt(d2);
          alpha_best_distance_m = (int)(M.m_distance_alfa / systemInfo.sample_rate / 1000);
        }

        real_distance[0] = (int)d2;

        getLocalTime(&tmstruct,0);
        time_hour[0] = tmstruct.tm_hour;
        time_min [0] = tmstruct.tm_min;
        time_sec [0] = tmstruct.tm_sec;

        this_run[0]      = alfa_counter;
        avg_speed[0]     = alfa_speed_max;
        message_nr[0]    = nav_pvt_message;
        alfa_distance[0] = (int)(M.m_distance_alfa / systemInfo.sample_rate);
      }
    }
  }

  // ---------------------------------------------------------------------------
  // FINALISE ON RUN CHANGE (RP6 / Speedreader-compatible)
  // ---------------------------------------------------------------------------
  if(run_count != old_run_count)
  {
    sort_run_results(
      avg_speed,
      real_distance,
      message_nr,
      time_hour,
      time_min,
      time_sec,
      this_run,
      alfa_distance,
      10
    );

    alfa_speed     = 0.0;
    alfa_speed_max = 0.0;
  }

  old_run_count = run_count;

  display_max_speed =
    (alfa_speed_max > avg_speed[9]) ? alfa_speed_max : avg_speed[9];

  return alfa_speed_max;
}

// -----------------------------------------------------------------------------
// Reset
// -----------------------------------------------------------------------------
void Alfa_speed::Reset_stats()
{
  for(int i=0;i<10;i++) {
    avg_speed[i] = 0.0;
    real_distance[i] = 0;
    time_hour[i] = 0;
    time_min[i] = 0;
    time_sec[i] = 0;
    this_run[i] = 0;
    message_nr[i] = 0;
    alfa_distance[i] = 0;
  }
  alfa_speed     = 0.0;
  alfa_speed_max = 0.0;
  display_max_speed = 0.0f;
  old_run_count = -1;
  alpha_best_speed_mmps = 0.0f;
  alpha_best_closure_m = 0.0f;
  alpha_best_distance_m = 0;
  alpha_start = -1;
  alpha_end = -1;
  alpha_sbp_start = -1;
  alpha_sbp_end = -1;
}

// -----------------------------------------------------------------------------
// Finalise_Run (explicit flush if needed)
// -----------------------------------------------------------------------------
void Alfa_speed::Finalise_Run()
{
  if(alfa_speed_max <= 0.0) return;

  sort_run_results(
    avg_speed,
    real_distance,
    message_nr,
    time_hour,
    time_min,
    time_sec,
    this_run,
    alfa_distance,
    10
  );

  display_max_speed = avg_speed[9];
}
