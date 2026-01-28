#include "GPS/gps_alpha.h"
#include "GPS/gps_speed.h"
#include "GPS/GPS_data.h"
#include "GPS/gps_geometry.h"


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
    sort_run_results(avg_speed, real_distance, message_nr,
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



/*hier wordt de actuele "alfa afstand" berekend aan de hand van 2 punten voor de gijp : P1 = 250m en P2 = 100m voor de gijp
*Deze punten bepalen een imaginaire lijn, de loodrechte afstand tot de actuele positie moet kleiner zijn dan 50 m/s
*als het punt P1 gepasseerd wordt
*/
double delta_heading;
double ref_heading;
float Alfa_indicator(GPS_speed M250,GPS_speed M100,float actual_heading){
  static float P1_lat,P1_long,P2_lat,P2_long;
  float P_lat,P_long, P_lat_heading,P_long_heading;
  //,lambda_T,lambda_N,lambda,
  float alfa_afstand;
  static int old_alfa_counter;
  if(alfa_counter!=old_alfa_counter){
    Ublox.alfa_distance=0;//afstand afgelegd sinds jibe detectie      10*100.000/10.000=100 samples ?
    P1_lat=_lat[M250.m_index%BUFFER_ALFA];//dit is het punt op -250 m van de actuele positie
    P1_long=_long[M250.m_index%BUFFER_ALFA];
    P2_lat=_lat[M100.m_index%BUFFER_ALFA];//dit is het punt op -100 m van de actuele positie (snelheid extrapolatie van -250m)
    P2_long=_long[M100.m_index%BUFFER_ALFA]; 
    }
  old_alfa_counter=alfa_counter;  
  P_lat=_lat[index_GPS%BUFFER_ALFA];//actuele positie lat
  P_long=_long[index_GPS%BUFFER_ALFA];//actuele positie long
  P_lat_heading= _lat[(index_GPS-2*systemInfo.sample_rate)%BUFFER_ALFA];//-2s  positie lat         //cos(ubxMessage.navPvt.heading*PI/180.0f/100000.0f)*111120+P_lat;//was eerst sin,extra punt berekenen heading, berekenen met afstand/lengte graad !!
  P_long_heading=_long[(index_GPS-2*systemInfo.sample_rate)%BUFFER_ALFA];//-2s  positie long//sin(ubxMessage.navPvt.heading*PI/180.0f/100000.0f)*111120*cos(DEG2RAD*P_lat)+P_long;//berekenen met afstand/lengte graad!!
  alfa_exit= Dis_point_line(P1_long,P1_lat,P_long,P_lat,P_long_heading,P_lat_heading);//
  alfa_afstand=Dis_point_line(P_long,P_lat,P1_long,P1_lat,P2_long,P2_lat);
  return alfa_afstand;  //actuele loodrechte afstand tov de lijn P2-P1, mag max 50m zijn voor een geldige alfa !!
}


