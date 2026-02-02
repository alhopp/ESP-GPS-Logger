// ============================================================================
// gps_alpha.cpp
//
// Alpha 500 — GYBE-CENTRIC, STRADDLE WINDOW, SBP-CORRECT
//
// RULES:
// - Must STRADDLE gybe: entry < alpha_gybe_index < exit
// - EXIT = current index_GPS
// - ENTRY slides backwards from gybe-1
// - Sailed distance (entry → exit) ≤ 500 m
// - Straight-line closure ≤ alfa_radius (50 m)
// - Speed = average of PER-SAMPLE speeds:
//     cm/s → knots → average
//
// UNITS:
// - Speed samples : cm/s → knots
// - Distance      : cm / m
// - Geometry      : meters
// ============================================================================

#include "GPS/gps_alpha.h"
#include "GPS/GPS_data.h"
#include "GPS/gps_utils.h"

#include "Core/Globals.h"
#include "core/system_info.h"

#include <math.h>
#include <time.h>

// -----------------------------------------------------------------------------
// External shared GPS state
// -----------------------------------------------------------------------------
extern int      index_GPS;
extern float    _lat[BUFFER_ALFA];
extern float    _long[BUFFER_ALFA];
extern uint16_t _sogCms[BUFFER_SIZE];
extern uint32_t _distCm[BUFFER_SIZE];

// From gps_run.cpp
extern volatile int alpha_gybe_index;
extern int alfa_counter;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
static inline int modA(int i){
  i %= BUFFER_ALFA;
  return (i < 0) ? i + BUFFER_ALFA : i;
}

// cm/s → knots (convert BEFORE averaging)
static inline float cms_to_kn(uint16_t cms){
  // 1 kn = 0.514444 m/s ; cms → m/s = cms / 100
  return (float)cms * 0.0194384449f;
}

// Fast local-plane closure distance² (m²)
static inline double closure_dist2(int a, int b){
  const double lat0 = _lat[a], lon0 = _long[a];
  const double lat1 = _lat[b], lon1 = _long[b];
  const double latm = 0.5 * (lat0 + lat1);
  const double dlat = (lat1 - lat0);
  const double dlon = (lon1 - lon0) * cos(DEG2RAD * latm);
  const double k = 111195.0;
  return (dlat*dlat + dlon*dlon) * k * k;
}

// ============================================================================
// Alfa_speed
// ============================================================================
Alfa_speed::Alfa_speed(int alfa_radius){
  alfa_circle_square = alfa_radius * alfa_radius; // m²
}

// -----------------------------------------------------------------------------
// Update_Alfa
// -----------------------------------------------------------------------------
float Alfa_speed::Update_Alfa(const GPS_speed& /*unused*/)
{
  static int last_alfa_counter = -1;

  // ---------------------------------------------------------------------------
  // Reset ONLY on new gybe
  // ---------------------------------------------------------------------------
  if(alfa_counter != last_alfa_counter){
    alfa_speed_max    = 0.0f;
    alfa_speed        = 0.0f;
    last_alfa_counter = alfa_counter;
  }

  const int g = alpha_gybe_index;
  if(g < 0) return alfa_speed_max;

  const int exit = index_GPS;
  if(exit <= g + 1) return alfa_speed_max;

  const int exitA = modA(exit);
  const int exitS = exit % BUFFER_SIZE;

  // ---------------------------------------------------------------------------
  // Precompute post-gybe contribution [g+1 → exit]
  // ---------------------------------------------------------------------------
  float post_sum_kn = 0.0f;
  int   post_n      = 0;

  for(int k = g + 1; k <= exit; k++){
    post_sum_kn += cms_to_kn(_sogCms[k % BUFFER_SIZE]);
    post_n++;
  }

  // ---------------------------------------------------------------------------
  // Slide ENTRY backwards from gybe-1
  // ---------------------------------------------------------------------------
  float pre_sum_kn = 0.0f;
  int   pre_n      = 0;

  for(int entry = g - 1; entry >= 0 && (g - entry) < BUFFER_SIZE; entry--)
  {
    // Add this entry sample
    pre_sum_kn += cms_to_kn(_sogCms[entry % BUFFER_SIZE]);
    pre_n++;

    // Sailed distance (cm → m)
    const uint32_t d0 = _distCm[entry % BUFFER_SIZE];
    const uint32_t d1 = _distCm[exitS];
    if(d1 <= d0) continue;

    const float sailed_m = (float)(d1 - d0) * 0.01f;
    if(sailed_m > 500.0f) break; // HARD STOP (earlier entries only get worse)

    // Closure test
    const int entryA = modA(entry);
    const double dist2 = closure_dist2(entryA, exitA);
    if(dist2 > alfa_circle_square) continue;

    // Average speed (knots)
    const int n = pre_n + post_n;
    if(n <= 0) continue;

    const float avg_kn = (pre_sum_kn + post_sum_kn) / (float)n;
    if(avg_kn <= alfa_speed_max) continue;

    // -------------------------------------------------------------------------
    // New best Alpha
    // -------------------------------------------------------------------------
    alfa_speed_max = avg_kn;
    alfa_speed     = avg_kn;

    real_distance[0] = (int)(sqrt(dist2) + 0.5f);

    getLocalTime(&tmstruct, 0);
    time_hour[0] = tmstruct.tm_hour;
    time_min [0] = tmstruct.tm_min;
    time_sec [0] = tmstruct.tm_sec;

    this_run[0]   = alfa_counter;
    avg_speed[0]  = alfa_speed_max;
    message_nr[0] = nav_pvt_message;

    alfa_distance[0] = (int)(sailed_m + 0.5f);
  }

  display_max_speed =
    (alfa_speed_max > avg_speed[0]) ? alfa_speed_max : avg_speed[0];

  return alfa_speed_max;
}

// -----------------------------------------------------------------------------
// Reset
// -----------------------------------------------------------------------------
void Alfa_speed::Reset_stats(){
  for(int i = 0; i < 10; i++) avg_speed[i] = 0;
}
