// ============================================================================
// gps_alpha.cpp
//
// Alpha (jibe) speed calculation — SBP / GPS-Speed matched
//
// RULES:
// - Alpha must STRADDLE a gybe boundary
// - Entry index < alpha_gybe_index
// - Exit  index >= alpha_gybe_index
// - Closure (entry → exit) <= 50 m
// - Speed comes DIRECTLY from GPS_speed::m_speed_alfa
// - Entry is SLID to find best valid sub-window
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
extern uint16_t _gSpeed[BUFFER_SIZE];
extern float    _lat[BUFFER_ALFA];
extern float    _long[BUFFER_ALFA];
extern int      index_GPS;

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

// alfa_radius = closure radius (50 m)
Alfa_speed::Alfa_speed(int alfa_radius){
  alfa_circle_square = alfa_radius * alfa_radius;
}

// -----------------------------------------------------------------------------
// Update_Alfa
// -----------------------------------------------------------------------------
float Alfa_speed::Update_Alfa(GPS_speed M)
{
  // No gybe yet → no Alpha possible
  if(alpha_gybe_index < 0)
    return alfa_speed_max;

  float  best_alpha = 0.0f;
  int    best_entry = -1;
  double best_dist2 = 0.0;

  const int exit_index = index_GPS;
  const int exit_i     = modA(exit_index);

  // ---------------------------------------------------------------------------
  // Slide ENTRY point on the PRE-GYBE side only
  // ---------------------------------------------------------------------------
  for(int entry = M.m_index + 1; entry < alpha_gybe_index; entry++)
  {
    const int entry_i = modA(entry);

    // ---------------------------------------------------------
    // Closure distance (entry → exit)
    // ---------------------------------------------------------
    const double lat0 = _lat[entry_i];
    const double lon0 = _long[entry_i];
    const double lat1 = _lat[exit_i];
    const double lon1 = _long[exit_i];

    const double latm = 0.5 * (lat0 + lat1);
    const double dlat = (lat1 - lat0);
    const double dlon = (lon1 - lon0) * cos(DEG2RAD * latm);

    const double dist2 =
      (dlat*dlat + dlon*dlon) * 111195.0 * 111195.0;

    // Must close within 50 m
    if(dist2 >= alfa_circle_square)
      continue;

    // ---------------------------------------------------------
    // Speed comes DIRECTLY from GPS_speed
    // ---------------------------------------------------------
    float s = M.m_speed_alfa;

    if(M.m_sample >= BUFFER_ALFA)
      s = 0;

    if(s > best_alpha){
      best_alpha = s;
      best_entry = entry;
      best_dist2 = dist2;
    }
  }

  alfa_speed = best_alpha;

  // ---------------------------------------------------------------------------
  // New best Alpha for THIS RUN
  // ---------------------------------------------------------------------------
  if(alfa_speed > alfa_speed_max && best_entry >= 0)
  {
    alfa_speed_max = alfa_speed;

    // ---------------- DEBUG: SBP-style dump ----------------
    Serial.println("\n================ ALPHA WINDOW =================");
    Serial.printf("Run            : %d\n", alfa_counter);
    Serial.printf("Gybe index     : %d\n", alpha_gybe_index);
    Serial.printf("Entry index    : %d\n", best_entry);
    Serial.printf("Exit index     : %d\n", index_GPS);

    const int samples = index_GPS - best_entry + 1;
    const double time_s = (double)samples / systemInfo.sample_rate;

    Serial.printf("Samples        : %d\n", samples);
    Serial.printf("Elapsed time   : %.3f s\n", time_s);

    const double path_m =
      (double)M.m_distance_alfa / systemInfo.sample_rate / 1000.0;

    Serial.printf("Path distance  : %.3f m\n", path_m);
    Serial.printf("Closure dist   : %.3f m\n", sqrt(best_dist2));

    Serial.printf("Alpha speed(dev): %.3f kn\n",
      (double)alfa_speed * MMPS_TO_KNOTS);

    Serial.printf("Alpha speed(calc): %.3f kn\n",
      (path_m / time_s) * 1.943844);

    Serial.println("\nSample speeds (kn):");
    for(int i = best_entry; i <= index_GPS; i++){
      int k = i % BUFFER_SIZE;
      Serial.printf("  %4d : %.3f\n",
        i - best_entry,
        (double)_gSpeed[k] * MMPS_TO_KNOTS);
    }
    Serial.println("==============================================\n");
    // -------------------------------------------------------

    real_distance[0] = (int)(sqrt(best_dist2) + 0.5f);

    getLocalTime(&tmstruct,0);
    time_hour[0] = tmstruct.tm_hour;
    time_min [0] = tmstruct.tm_min;
    time_sec [0] = tmstruct.tm_sec;

    this_run[0]   = alfa_counter;
    avg_speed[0]  = alfa_speed_max;
    message_nr[0] = nav_pvt_message;

    alfa_distance[0] =
      M.m_distance_alfa / systemInfo.sample_rate;
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
// Reset ALFA statistics
// -----------------------------------------------------------------------------
void Alfa_speed::Reset_stats(){
  for(int i=0;i<10;i++) avg_speed[i]=0;
}

// ============================================================================
// Alfa_indicator (unchanged — geometry validity only)
// ============================================================================
double delta_heading;
double ref_heading;

float Alfa_indicator(GPS_speed M250, GPS_speed M100, float /*actual_heading*/)
{
  static float P1_lat,P1_long,P2_lat,P2_long;
  static int   old_alfa_counter;

  float P_lat,P_long,P_lat_heading,P_long_heading;
  float alfa_afstand;

  if(alfa_counter != old_alfa_counter){
    Ublox.alfa_distance = 0;
    P1_lat  = _lat [modA(M250.m_index)];
    P1_long = _long[modA(M250.m_index)];
    P2_lat  = _lat [modA(M100.m_index)];
    P2_long = _long[modA(M100.m_index)];
  }
  old_alfa_counter = alfa_counter;

  P_lat  = _lat [modA(index_GPS)];
  P_long = _long[modA(index_GPS)];

  P_lat_heading  = _lat [modA(index_GPS - 2*systemInfo.sample_rate)];
  P_long_heading = _long[modA(index_GPS - 2*systemInfo.sample_rate)];

  alfa_exit = Dis_point_line(
    P1_long, P1_lat,
    P_long,  P_lat,
    P_long_heading, P_lat_heading
  );

  alfa_afstand = Dis_point_line(
    P_long, P_lat,
    P1_long, P1_lat,
    P2_long, P2_lat
  );

  return alfa_afstand;
}
