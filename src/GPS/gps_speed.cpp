#include "GPS/gps_speed.h"
#include "GPS/GPS_data.h"        
#include "GPS/gps_utils.h"        

// _gSpeed[], index_GPS, nav_pvt_message
#include "core/system_info.h"    // systemInfo.sample_rate
#include "Ublox/ublox.h"         // Ublox globals (SBP / NAV context)
#include "Core/Globals.h"        // tmstruct, getLocalTime()
#include <time.h>

// -----------------------------------------------------------------------------
// GPS_speed
// Distance-based average speed calculator (e.g. 100m / 250m / 500m / 1852m)
//
// - Integrates Doppler speed samples (mm/s)
// - Maintains a sliding distance window
// - Tracks best average speed per run + top-10 history
// -----------------------------------------------------------------------------

GPS_speed::GPS_speed(int afstand)
: m_set_distance(afstand) // distance window in meters
{}

// -----------------------------------------------------------------------------
// Update_distance
// Called every GPS sample.
// Returns current best speed (mm/s) for this distance window.
// -----------------------------------------------------------------------------
double GPS_speed::Update_distance(int actual_run)
{
  // Target distance in mm, scaled by sample rate
  m_Set_Distance = m_set_distance * 1000 * systemInfo.sample_rate;

  // Accumulate distance using Doppler speed
  m_distance += _gSpeed[index_GPS % BUFFER_SIZE];

  // Buffer overflow protection
  if ((index_GPS - m_index) >= BUFFER_SIZE) {
    m_distance = 0;
    m_index    = index_GPS;
  }

  // Slide window until exact distance window is reached
  if (m_distance > m_Set_Distance) {
    while (m_distance > m_Set_Distance && (index_GPS - m_index) < BUFFER_SIZE) {
      m_distance      -= _gSpeed[m_index % BUFFER_SIZE];
      m_distance_alfa  = m_distance;   // saved for alpha calculation
      m_index++;
    }
    m_index--;
    m_distance += _gSpeed[m_index % BUFFER_SIZE];
  }

  // Number of samples in window
  m_sample = index_GPS - m_index + 1;

  // Average speed (mm/s)
  if (m_sample)
    m_speed = (double)m_distance / m_sample;

  if (index_GPS - m_index)
    m_speed_alfa = (double)m_distance_alfa / (index_GPS - m_index);

    

  // Invalid until full distance reached
  if (m_distance < m_Set_Distance || m_sample >= BUFFER_SIZE) m_speed = 0;
  if (m_speed == 0) m_speed_alfa = 0;

  // New max speed detected
  if (m_max_speed < m_speed) {
    m_max_speed = m_speed;

    getLocalTime(&tmstruct, 0);
    time_hour[0] = tmstruct.tm_hour;
    time_min[0]  = tmstruct.tm_min;
    time_sec[0]  = tmstruct.tm_sec;

    this_run[0]   = actual_run;
    avg_speed[0]  = m_max_speed;
    m_Distance[0] = m_distance;
    nr_samples[0] = m_sample;
    message_nr[0] = nav_pvt_message;

    // Update live sorted display
    for (int i = 0; i < 10; i++) display_speed[i] = avg_speed[i];
    sort_display(display_speed, 10);
  }

  // Run boundary → archive previous run result
  if (actual_run != old_run && this_run[0] == old_run) {
    sort_run_results(avg_speed, m_Distance, message_nr,
                  time_hour, time_min, time_sec,
                  this_run, nr_samples, 10);
    avg_speed[0] = 0;
    m_max_speed  = 0;
  }

  old_run = actual_run;
  return m_max_speed;
}



