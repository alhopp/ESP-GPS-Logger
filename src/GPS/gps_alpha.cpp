#include "GPS/gps_alpha.h"
#include "GPS/gps_speed.h"
#include "GPS/GPS_data.h"

#include "Core/Globals.h"
#include "core/system_info.h"
#include "Ublox/ublox.h"

#include <math.h>
#include <string.h>
#include <time.h>

// -----------------------------------------------------------------------------
// Alfa_speed
// -----------------------------------------------------------------------------
// Computes best ALFA speed:
// - Uses distance-based speed window (GPS_speed)
// - Valid ALFA if straight-line distance between entry/exit < alfa_radius
// - Tracks best result per run and stores top-10 sorted results
// -----------------------------------------------------------------------------

Alfa_speed::Alfa_speed(int alfa_radius){
  // Store squared radius (meters²) to avoid sqrt during comparisons
  alfa_circle_square = alfa_radius * alfa_radius;
}

/*
 * NOTE:
 * - m_speed_alfa is used instead of m_speed
 * - Valid only while covered distance < BUFFER_ALFA
 */
float Alfa_speed::Update_Alfa(GPS_speed M){
  // Squared straight-line distance between current point and start of window
  straight_dist_square =
    ( pow((_lat[index_GPS % BUFFER_ALFA] - _lat[(M.m_index + 1) % BUFFER_ALFA]), 2) +
      pow(cos(DEG2RAD * _lat[index_GPS % BUFFER_ALFA]) *
          (_long[index_GPS % BUFFER_ALFA] - _long[(M.m_index + 1) % BUFFER_ALFA]), 2)
    ) * 111195 * 111195;

  // Check ALFA geometry constraint
  if(straight_dist_square < alfa_circle_square){
    alfa_speed = M.m_speed_alfa;
    if(M.m_sample >= BUFFER_ALFA) alfa_speed = 0; // overflow guard

    // New ALFA max
    if(alfa_speed > alfa_speed_max){
      alfa_speed_max = alfa_speed;
      real_distance[0] = (int)straight_dist_square;

      getLocalTime(&tmstruct, 0);
      time_hour[0] = tmstruct.tm_hour;
      time_min[0]  = tmstruct.tm_min;
      time_sec[0]  = tmstruct.tm_sec;

      this_run[0]   = alfa_counter;
      avg_speed[0]  = alfa_speed_max;
      message_nr[0] = nav_pvt_message;
      alfa_distance[0] = M.m_distance_alfa / systemInfo.sample_rate;
    }
  }

  // End-of-run → sort + reset
  if(run_count != old_run_count){
    sort_run_alfa(avg_speed, real_distance, message_nr,
                  time_hour, time_min, time_sec,
                  alfa_distance, this_run, 10);

    alfa_speed = 0;
    alfa_speed_max = 0;
  }

  old_run_count = run_count;

  // Live display max
  display_max_speed =
    (alfa_speed_max > avg_speed[9]) ? alfa_speed_max : avg_speed[9];

  return alfa_speed_max;
}

// -----------------------------------------------------------------------------
// Reset
// -----------------------------------------------------------------------------
void Alfa_speed::Reset_stats(void){
  for(int i = 0; i < 10; i++) avg_speed[i] = 0;
}
