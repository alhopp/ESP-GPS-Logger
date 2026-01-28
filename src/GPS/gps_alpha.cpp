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

// Debug / UI: signed distance to alpha exit line
float alfa_exit;

// ============================================================================
// Alfa_speed
//
// Computes ALFA (jibe-based) speed results.
//
// Concept:
// - Uses distance-based speed windows (GPS_speed)
// - Checks ALFA geometry constraint:
//     straight-line distance between entry and exit must be < alfa_radius
// - Tracks best ALFA per run
// - Stores + sorts top-10 ALFA results at end of each run
//
// Dependencies:
// - Position buffers (_lat/_long) from GPS_data
// - Run/jibe counters from New_run_detection()
// - Speed windows from gps_speed
// ============================================================================

Alfa_speed::Alfa_speed(int alfa_radius){
  // Store radius² (meters²) to avoid sqrt during runtime
  alfa_circle_square = alfa_radius * alfa_radius;
}




/*
 * Update_Alfa
 *
 * Called every GPS sample.
 * - Evaluates ALFA geometry using short position history (BUFFER_ALFA)
 * - Updates max ALFA speed for current run
 * - On run end: sorts results and resets state
 */
float Alfa_speed::Update_Alfa(GPS_speed M)
{
  // Straight-line distance² between:
  //  - current position
  //  - start of speed window (M.m_index)
  straight_dist_square =
    ( pow(_lat[index_GPS % BUFFER_ALFA] -
          _lat[(M.m_index + 1) % BUFFER_ALFA], 2) +
      pow(cos(DEG2RAD * _lat[index_GPS % BUFFER_ALFA]) *
          (_long[index_GPS % BUFFER_ALFA] -
           _long[(M.m_index + 1) % BUFFER_ALFA]), 2)
    ) * 111195 * 111195; // deg → meters

  // Geometry constraint: must fit inside ALFA circle
  if(straight_dist_square < alfa_circle_square){
    alfa_speed = M.m_speed_alfa;
    if(M.m_sample >= BUFFER_ALFA) alfa_speed = 0; // safety guard

    // New best ALFA for this run
    if(alfa_speed > alfa_speed_max){
      alfa_speed_max = alfa_speed;
      real_distance[0] = (int)straight_dist_square;

      getLocalTime(&tmstruct,0);
      time_hour[0] = tmstruct.tm_hour;
      time_min [0] = tmstruct.tm_min;
      time_sec [0] = tmstruct.tm_sec;

      this_run[0]      = alfa_counter;
      avg_speed[0]     = alfa_speed_max;
      message_nr[0]    = nav_pvt_message;
      alfa_distance[0] = M.m_distance_alfa / systemInfo.sample_rate;
    }
  }

  // Run finished → sort results & reset run-local state
  if(run_count != old_run_count){
    sort_run_results(avg_speed, real_distance, message_nr,
                     time_hour, time_min, time_sec,
                     alfa_distance, this_run, 10);

    alfa_speed = alfa_speed_max = 0;
    
  }

  old_run_count = run_count;

  // Live display value = best of current run vs stored top-10
  display_max_speed =
    (alfa_speed_max > avg_speed[9]) ? alfa_speed_max : avg_speed[9];

  return alfa_speed_max;
}

// -----------------------------------------------------------------------------
// Reset ALFA statistics (called on session reset)
// -----------------------------------------------------------------------------
void Alfa_speed::Reset_stats(){
  for(int i=0;i<10;i++) avg_speed[i]=0;
}


// ============================================================================
// Alfa_indicator
//
// Computes perpendicular distance to the ALFA reference line.
//
// Geometry:
// - P1: position ~250 m before jibe
// - P2: position ~100 m before jibe
// - Line P1–P2 defines the reference axis
// - Current position must remain within tolerance of this line
//
// Role:
// - Used as *indicator / validity check*, NOT speed computation
// - Called continuously after jibe detection
//
// Dependencies:
// - alfa_counter from New_run_detection()
// - Position buffers (_lat/_long)
// - gps_speed windows for index backtracking
// ============================================================================

double delta_heading;
double ref_heading;

float Alfa_indicator(GPS_speed M250, GPS_speed M100, float /*actual_heading*/)
{
  static float P1_lat,P1_long,P2_lat,P2_long;
  static int   old_alfa_counter;

  float P_lat,P_long,P_lat_heading,P_long_heading;
  float alfa_afstand;

  // New jibe → capture reference points
  if(alfa_counter != old_alfa_counter){
    Ublox.alfa_distance = 0;

    P1_lat  = _lat [M250.m_index % BUFFER_ALFA]; // ~250 m before
    P1_long = _long[M250.m_index % BUFFER_ALFA];
    P2_lat  = _lat [M100.m_index % BUFFER_ALFA]; // ~100 m before
    P2_long = _long[M100.m_index % BUFFER_ALFA];
  }
  old_alfa_counter = alfa_counter;

  // Current position
  P_lat  = _lat [index_GPS % BUFFER_ALFA];
  P_long = _long[index_GPS % BUFFER_ALFA];

  // Heading reference (~2 seconds back)
  P_lat_heading  = _lat [(index_GPS - 2*systemInfo.sample_rate) % BUFFER_ALFA];
  P_long_heading = _long[(index_GPS - 2*systemInfo.sample_rate) % BUFFER_ALFA];

  // Signed distance to heading-aligned line (debug / UI)
  alfa_exit = Dis_point_line(
    P1_long, P1_lat,
    P_long,  P_lat,
    P_long_heading, P_lat_heading
  );

  // Perpendicular distance to P2–P1 reference line
  alfa_afstand = Dis_point_line(
    P_long, P_lat,
    P1_long, P1_lat,
    P2_long, P2_lat
  );

  // Must remain within tolerance to be valid ALFA
  return alfa_afstand;
}




