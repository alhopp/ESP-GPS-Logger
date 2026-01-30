#include "GPS/gps_time.h"
#include "GPS/GPS_data.h"
#include "GPS/gps_utils.h"        

#include "Ublox/ublox.h"
#include "Core/Globals.h"
#include "core/system_info.h"

#include <time.h>

// ============================================================================
// GPS_time
// Time-window based speed statistics (2s / 10s / 30m / 1h etc)
//
// - Consumes raw Doppler samples from GPS_data (_gSpeed / _secSpeed)
// - Computes rolling average speed over a fixed time window
// - Tracks session-best, per-run best, and ranked top-10 values
// ============================================================================

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------
GPS_time::GPS_time(int tijdvenster)
: time_window(tijdvenster)
{
  Reset_stats();
}

// -----------------------------------------------------------------------------
// Reset all rolling / ranked state (called on new session)
// -----------------------------------------------------------------------------
void GPS_time::Reset_stats(){
  for(int i=0;i<10;i++){ avg_speed[i]=0; display_speed[i]=0; }
  avg_5runs=0;
}

// -----------------------------------------------------------------------------
// Update_speed
// Called on every GPS sample
//
// actual_run : monotonically increasing run counter
// returns    : current max speed for this time window (mm/s)
// -----------------------------------------------------------------------------
float GPS_time::Update_speed(int actual_run)
{
  // ---------------------------------------------------------------------------
  // FAST PATH: window fits inside raw gSpeed buffer (high-rate calculation)
  // ---------------------------------------------------------------------------
  if(time_window*systemInfo.sample_rate < BUFFER_SIZE){

     // SBP parity: work in cm/s (integer)
    uint16_t cmps_now = _gSpeed[index_GPS % BUFFER_SIZE] / 10;

    avg_s_sum += cmps_now;

    if(index_GPS >= time_window * systemInfo.sample_rate){
      uint16_t cmps_old =
        _gSpeed[(index_GPS - time_window * systemInfo.sample_rate) % BUFFER_SIZE] / 10;
      avg_s_sum -= cmps_old;
    }

    // Integer average exactly like SBP
    uint32_t samples = time_window * systemInfo.sample_rate;
    uint16_t avg_cmps = avg_s_sum / samples;

    // Convert once
    avg_s = (double)avg_cmps; // STORE cm/s internally


    // New max detected
    if(s_max_speed < avg_s){
      s_max_speed = avg_s;
      speed_run[actual_run % NR_OF_BAR] = avg_s;

      getLocalTime(&tmstruct,0);
      time_hour[0]=tmstruct.tm_hour; time_min[0]=tmstruct.tm_min; time_sec[0]=tmstruct.tm_sec;
      this_run[0]=actual_run;
      avg_speed[0]=s_max_speed;

      Mean_cno[0]=Ublox_Sat.sat_info.Mean_mean_cno;
      Max_cno [0]=Ublox_Sat.sat_info.Mean_max_cno;
      Min_cno [0]=Ublox_Sat.sat_info.Mean_min_cno;
      Mean_numSat[0]=Ublox_Sat.sat_info.Mean_numSV;

      for(int i=0;i<10;i++) display_speed[i]=avg_speed[i];
      sort_display(display_speed,10);
      display_max_speed = display_speed[9];

      avg_5runs=0; for(int i=5;i<10;i++) avg_5runs+=display_speed[i];
      avg_5runs/=5;
    }

    // Run ended → archive best result
    if(actual_run!=old_run && this_run[0]==old_run){
      sort_run(avg_speed,time_hour,time_min,time_sec,
               Mean_cno,Max_cno,Min_cno,Mean_numSat,this_run,10);

      if(s_max_speed>500) speed_run_counter++;
      speed_run[actual_run%NR_OF_BAR]=avg_speed[0];

      avg_speed[0]=0; s_max_speed=0;
      avg_5runs=0; for(int i=5;i<10;i++) avg_5runs+=avg_speed[i];
      avg_5runs/=5;
    }

    if(actual_run!=reset_display_last_run && avg_s>3000){
      reset_display_last_run=actual_run; display_last_run=0;
    } else if(display_last_run<s_max_speed) display_last_run=s_max_speed;

    old_run=actual_run;
    return s_max_speed;
  }

  // ---------------------------------------------------------------------------
  // SLOW PATH: second-averaged buffer (used for long windows e.g. 30m / 1h)
  // ---------------------------------------------------------------------------
  if(index_GPS % systemInfo.sample_rate == 0){

    avg_s_sum += _secSpeed[index_sec % BUFFER_SIZE];
    if(index_sec >= time_window)
      avg_s_sum -= _secSpeed[(index_sec - time_window) % BUFFER_SIZE];

    avg_s = (double)avg_s_sum / time_window;

    if(s_max_speed < avg_s){
      s_max_speed = avg_s;
      getLocalTime(&tmstruct,0);
      time_hour[0]=tmstruct.tm_hour; time_min[0]=tmstruct.tm_min; time_sec[0]=tmstruct.tm_sec;
      this_run[0]=actual_run;
      avg_speed[0]=s_max_speed;
    }

    display_max_speed = (s_max_speed>avg_speed[9]) ? s_max_speed : avg_speed[9];

    if(actual_run!=old_run && this_run[0]==old_run){
      sort_run(avg_speed,time_hour,time_min,time_sec,
               Mean_cno,Max_cno,Min_cno,Mean_numSat,this_run,10);
      avg_speed[0]=0; s_max_speed=0;
      avg_5runs=0; for(int i=5;i<10;i++) avg_5runs+=avg_speed[i];
      avg_5runs/=5;
    }

    old_run=actual_run;
    return s_max_speed;
  }

  return s_max_speed; // safety
}
