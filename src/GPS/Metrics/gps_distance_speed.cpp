#include "GPS/Metrics/gps_distance_speed.h"
#include "GPS/Data/gps_data.h"
#include "GPS/Metrics/gps_result_sort.h"

#include "Core/system_info.h"
#include "GPS/Ublox/ublox_driver.h"
#include "Core/Globals.h"
#include "GPS/gps_runtime_state.h"
#include <time.h>

// -----------------------------------------------------------------------------
// GPS_distance_speed
// Distance-based average speed calculator (100m / 250m / 500m / 1852m)
//
// UNIT MODEL (RP6 / Speedreader-aligned):
// - _gSpeed is raw mm/s.
// - Window distance uses the legacy scaled target:
//     meters * 1000 * sample_rate
// - Stored speeds remain mm/s. Display/export code converts to knots.
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Geometry window export (NM only for now)
// -----------------------------------------------------------------------------
int win_nm_start = -1;
int win_nm_end   = -1;

GPS_distance_speed::GPS_distance_speed(int afstand) : m_set_distance(afstand){}

// -----------------------------------------------------------------------------
double GPS_distance_speed::Update_distance(int actual_run)
{
  m_Set_Distance = m_set_distance * 1000 * systemInfo.sample_rate;

  if(index_GPS == 0){
    m_distance = 0;
    m_index    = 0;
    m_sample   = 0;
    old_run    = actual_run;
    return m_max_speed;
  }

  m_distance += _gSpeed[index_GPS % BUFFER_SIZE];

  if((index_GPS - m_index) >= BUFFER_SIZE){
    m_distance = 0;
    m_index    = index_GPS;
  }

  if(m_distance > m_Set_Distance){
    while(m_distance > m_Set_Distance && (index_GPS - m_index) < BUFFER_SIZE){
      m_distance     -= _gSpeed[m_index % BUFFER_SIZE];
      m_distance_alfa = m_distance;
      m_index++;
    }
    m_index--;
    m_distance += _gSpeed[m_index % BUFFER_SIZE];
  }

  m_sample = index_GPS - m_index + 1;
  if(m_sample <= 0) return m_max_speed;

  if(m_distance < m_Set_Distance || m_sample >= BUFFER_SIZE){
    m_speed = 0.0;
  }else{
    m_speed = (double)m_distance / (double)m_sample;
  }

  if(m_speed == 0.0 || (index_GPS - m_index) <= 0){
    m_speed_alfa = 0.0;
  }else{
    m_speed_alfa = (double)m_distance_alfa / (double)(index_GPS - m_index);
  }

  // ---------------------------------------------------------------------------
  // New best speed → CAPTURE GEOMETRY WINDOW
  // ---------------------------------------------------------------------------
  if((m_max_speed == 0.0 && m_speed > 0.0) || m_speed > m_max_speed){
    m_max_speed = m_speed;

    // ---- Capture NM geometry window (1852 m only) ----
    if(m_set_distance == 1852){
      win_nm_start = m_index;
      win_nm_end   = index_GPS;
    }

    getLocalTime(&tmstruct,0);
    time_hour[0] = tmstruct.tm_hour;
    time_min [0] = tmstruct.tm_min;
    time_sec [0] = tmstruct.tm_sec;

    this_run[0]   = actual_run;
    avg_speed[0]  = m_max_speed;
    m_Distance[0] = m_distance;
    nr_samples[0] = m_sample;
    message_nr[0] = nav_pvt_message;

    for(int i=0;i<10;i++) display_speed[i] = avg_speed[i];
    sort_display(display_speed,10);
  }

  // ---------------------------------------------------------------------------
  // Run boundary
  // ---------------------------------------------------------------------------
  if(actual_run != old_run && this_run[0] == old_run){
    sort_run_results(
      avg_speed,
      m_Distance,
      message_nr,
      time_hour,
      time_min,
      time_sec,
      this_run,
      nr_samples,
      10
    );
    avg_speed[0] = 0.0;
    m_max_speed  = 0.0;
  }

  old_run = actual_run;
  return m_max_speed;
}
