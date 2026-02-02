// ============================================================================
// gps_alpha.cpp
//
// Alpha (jibe) speed calculation — SBP / GPS-Speed matched
//
// RULES:
// - Alpha must STRADDLE a gybe boundary (alpha_gybe_index)
// - Entry index < alpha_gybe_index
// - Exit  index >= alpha_gybe_index
// - Closure (entry → exit) <= 50 m
// - Speed comes DIRECTLY from GPS_speed::m_speed_alfa (KNOTS)
// - Entry is SLID to find best valid sub-window
//
// UNIT CONTRACT (CURRENT):
// - GPS_speed::m_speed_alfa is already KNOTS (per-sample converted before avg)
// - Comparisons / ranking / stored results are KNOTS
// ============================================================================

#include "GPS/gps_alpha.h"
#include "GPS/gps_speed.h"
#include "GPS/GPS_data.h"
#include "GPS/gps_geometry.h"
#include "GPS/gps_utils.h"

#include "Core/Globals.h"
#include "core/system_info.h"
#include "Ublox/ublox.h"

#include <math.h>
#include <time.h>

// -----------------------------------------------------------------------------
// External shared GPS state
// -----------------------------------------------------------------------------
extern float _lat[BUFFER_ALFA];
extern float _long[BUFFER_ALFA];
extern int   index_GPS;

// From New_run_detection()
extern volatile int alpha_gybe_index;

// -----------------------------------------------------------------------------
// Debug / UI
// -----------------------------------------------------------------------------
float alfa_exit;

// Safe modulo for BUFFER_ALFA
static inline int modA(int i){
  i %= BUFFER_ALFA;
  return (i < 0) ? (i + BUFFER_ALFA) : i;
}

// ============================================================================
// Alfa_speed
// ============================================================================

Alfa_speed::Alfa_speed(int alfa_radius){
  alfa_circle_square = alfa_radius * alfa_radius; // meters²
}

// -----------------------------------------------------------------------------
// Update_Alfa
// -----------------------------------------------------------------------------
float Alfa_speed::Update_Alfa(const GPS_speed& M)
{
  // No gybe yet → no Alpha possible
  if(alpha_gybe_index < 0) return alfa_speed_max;

  float  best_alpha_kn = 0.0f;
  int    best_entry    = -1;
  double best_dist2    = 0.0;

  const int exit_index = index_GPS;
  const int exit_i     = modA(exit_index);

  // ---------------------------------------------------------------------------
  // Slide ENTRY point on the PRE-GYBE side only
  // ---------------------------------------------------------------------------
  for(int entry = M.m_index + 1; entry < alpha_gybe_index; entry++)
  {
    const int entry_i = modA(entry);

    // Closure distance (entry → exit) in meters² (fast local-plane approx)
    const double lat0 = _lat[entry_i];
    const double lon0 = _long[entry_i];
    const double lat1 = _lat[exit_i];
    const double lon1 = _long[exit_i];

    const double latm = 0.5 * (lat0 + lat1);
    const double dlat = (lat1 - lat0);
    const double dlon = (lon1 - lon0) * cos(DEG2RAD * latm);

    const double dist2 = (dlat*dlat + dlon*dlon) * 111195.0 * 111195.0;

    // Must close within 50 m
    if(dist2 >= alfa_circle_square) continue;

    // Speed comes DIRECTLY from GPS_speed (already KNOTS)
    float s_kn = (M.m_sample >= BUFFER_ALFA) ? 0.0f : (float)M.m_speed_alfa;

    if(s_kn > best_alpha_kn){
      best_alpha_kn = s_kn;
      best_entry    = entry;
      best_dist2    = dist2;
    }
  }

  alfa_speed = best_alpha_kn; // knots

  // ---------------------------------------------------------------------------
  // New best Alpha for THIS RUN
  // ---------------------------------------------------------------------------
  if(alfa_speed > alfa_speed_max && best_entry >= 0)
  {
    alfa_speed_max = alfa_speed;

    // Optional: keep your SBP-style dump (still valid)
    Serial.println("\n================ ALPHA WINDOW =================");
    Serial.printf("Run            : %d\n", alfa_counter);
    Serial.printf("Gybe index     : %d\n", alpha_gybe_index);
    Serial.printf("Entry index    : %d\n", best_entry);
    Serial.printf("Exit index     : %d\n", index_GPS);

    const int samples   = index_GPS - best_entry + 1;
    const double time_s = (double)samples / systemInfo.sample_rate;

    Serial.printf("Samples        : %d\n", samples);
    Serial.printf("Elapsed time   : %.3f s\n", time_s);
    Serial.printf("Closure dist   : %.3f m\n", sqrt(best_dist2));
    Serial.printf("Alpha speed    : %.3f kn\n", (double)alfa_speed_max);
    Serial.println("==============================================\n");

    real_distance[0] = (int)(sqrt(best_dist2) + 0.5f);

    getLocalTime(&tmstruct,0);
    time_hour[0] = tmstruct.tm_hour;
    time_min [0] = tmstruct.tm_min;
    time_sec [0] = tmstruct.tm_sec;

    this_run[0]   = alfa_counter;
    avg_speed[0]  = alfa_speed_max;     // knots
    message_nr[0] = nav_pvt_message;

    // Keep this as-is if you still want it (distance-ish metadata)
    alfa_distance[0] = M.m_distance_alfa / systemInfo.sample_rate;
  }

  // ---------------------------------------------------------------------------
  // Run finished → archive & reset
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
      alfa_distance,
      this_run,
      10
    );

    alfa_speed = 0;
    alfa_speed_max = 0;
  }

  old_run_count = run_count;

  display_max_speed =
    (alfa_speed_max > avg_speed[0]) ? alfa_speed_max : avg_speed[0];

  return alfa_speed_max;
}

// -----------------------------------------------------------------------------
void Alfa_speed::Reset_stats(){
  for(int i=0;i<10;i++) avg_speed[i]=0;
}
