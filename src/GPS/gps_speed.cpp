#include "GPS/gps_speed.h"
#include "GPS/GPS_data.h"
#include "GPS/gps_utils.h"

#include "core/system_info.h"
#include "Ublox/ublox.h"
#include "Core/Globals.h"
#include <time.h>

// -----------------------------------------------------------------------------
// GPS_speed
// Distance-based average speed calculator (100m / 250m / 500m / 1852m)
//
// UNIT MODEL (Speedreader-aligned):
// - Sample 0 initializes state ONLY (no distance contribution)
// - Distance integration starts at sample 1
// - Distance: mm per sample (_gSpeed / sample_rate)
// - Speed samples: cm/s → knots BEFORE averaging
// - Averaging: FLOAT knots
// - Padding allowed for incomplete windows
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Geometry window export (NM only for now)
// -----------------------------------------------------------------------------
int win_nm_start = -1;
int win_nm_end   = -1;

GPS_speed::GPS_speed(int afstand) : m_set_distance(afstand){}

// -----------------------------------------------------------------------------
double GPS_speed::Update_distance(int actual_run)
{
  // Target distance in mm (meters → mm)
  m_Set_Distance = m_set_distance * 1000;

  // ---------------------------------------------------------------------------
  // Sample 0 = state init only (Speedreader behaviour)
  // ---------------------------------------------------------------------------
  if(index_GPS == 0){
    m_distance = 0.0;
    m_index    = 1;      // distance window starts at sample 1
    m_sample   = 0;
    old_run    = actual_run;
    return m_max_speed;
  }

  // ---------------------------------------------------------------------------
  // Distance integration
  // _gSpeed is mm/s → convert to mm per sample
  // ---------------------------------------------------------------------------
  m_distance += _gSpeed[index_GPS % BUFFER_SIZE] / systemInfo.sample_rate;

  // Overflow safety
  if((index_GPS - m_index) >= BUFFER_SIZE){
    m_distance = 0.0;
    m_index    = index_GPS;
  }

  // ---------------------------------------------------------------------------
  // Slide distance window
  // ---------------------------------------------------------------------------
  if(m_distance > m_Set_Distance){
    while(m_distance > m_Set_Distance && (index_GPS - m_index) < BUFFER_SIZE){
      m_distance     -= _gSpeed[m_index % BUFFER_SIZE] / systemInfo.sample_rate;
      m_distance_alfa = m_distance;
      m_index++;
    }
    m_index--;
    m_distance += _gSpeed[m_index % BUFFER_SIZE] / systemInfo.sample_rate;
  }

  // Sample count (distance-bearing samples only)
  m_sample = index_GPS - m_index + 1;
  if(m_sample <= 0) return m_max_speed;

  // ---------------------------------------------------------------------------
  // SBP-style averaging: cm/s → knots → average → padded
  // ---------------------------------------------------------------------------
  double speed_kn = 0.0;
  double alfa_kn  = 0.0;

  if(m_Set_Distance > 0){
    double sum_kn = 0.0;

    for(int i = m_index; i <= index_GPS; i++){
      int k = i % BUFFER_SIZE;
      sum_kn += (double)_sogCms[k] * CMPS_TO_KNOTS;
    }

    double avg_kn = sum_kn / m_sample;

    double completion = (double)m_distance / (double)m_Set_Distance;
    if(completion > 1.0) completion = 1.0;

    speed_kn = avg_kn * completion;
  }

  // ---------------------------------------------------------------------------
  // Alpha variant (same rule, excludes anchor sample)
  // ---------------------------------------------------------------------------
  if((index_GPS - m_index) > 0 && m_Set_Distance > 0){
    double sum_kn = 0.0;

    for(int i = m_index + 1; i <= index_GPS; i++){
      int k = i % BUFFER_SIZE;
      sum_kn += (double)_sogCms[k] * CMPS_TO_KNOTS;
    }

    double avg_kn = sum_kn / (index_GPS - m_index);

    double completion = (double)m_distance / (double)m_Set_Distance;
    if(completion > 1.0) completion = 1.0;

    alfa_kn = avg_kn * completion;
  }

  m_speed      = speed_kn;
  m_speed_alfa = alfa_kn;

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
