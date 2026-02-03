// ============================================================================
// gps_alpha.cpp
//
// Alpha 500 — LEGACY-COMPATIBLE IMPLEMENTATION (M500-driven)
//
// Behaviour (matches your legacy snippet):
// - Uses distance-based GPS_speed integrator for 500m window
// - Entry = (M.m_index + 1)
// - Exit  = current index_GPS
// - Closure < alfa_radius (50 m)
// - Speed = M.m_speed_alfa (SBP parity / distance-window exact)
// - Alpha is FINALIZED on run change
// ============================================================================

#include "GPS/gps_alpha.h"
#include "GPS/GPS_data.h"
#include "GPS/gps_speed.h"
#include "GPS/gps_utils.h"

#include "Core/Globals.h"
#include "core/system_info.h"

#include <Arduino.h>
#include <math.h>
#include <time.h>


#include "GPS/gps_alpha.h"

// ============================================================================
// Alfa_speed constructor
// ============================================================================
Alfa_speed::Alfa_speed(int alfa_radius)
{
  alfa_circle_square = (double)alfa_radius * (double)alfa_radius;

  // Initialise runtime state
  alfa_speed        = 0.0;
  alfa_speed_max    = 0.0;
  display_max_speed = 0.0;

  // Clear result arrays
  for(int i = 0; i < 10; i++){
    avg_speed[i]     = 0.0;
    real_distance[i]= 0;
    time_hour[i]     = 0;
    time_min[i]      = 0;
    time_sec[i]      = 0;
    this_run[i]      = 0;
    message_nr[i]    = 0;
    alfa_distance[i] = 0;
  }

  old_run_count = -1;

  Serial.printf("[ALFA] ctor radius=%dm\n", alfa_radius);
}


// -----------------------------------------------------------------------------
// External shared state
// -----------------------------------------------------------------------------
extern int index_GPS;
extern int alfa_counter;

// If gps_utils.h doesn't declare this in your build, keep this prototype here.
extern void sort_run_results(double a[], int dis[], int message[],
                             uint8_t hour[], uint8_t minute[], uint8_t seconde[],
                             int runs[], int samples[], int size);

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
static inline double closure_dist2(int a, int b)
{
  const double lat0 = _lat[a];
  const double lon0 = _long[a];
  const double lat1 = _lat[b];
  const double lon1 = _long[b];

  const double latm = 0.5 * (lat0 + lat1);
  const double dlat = lat1 - lat0;
  const double dlon = (lon1 - lon0) * cos(DEG2RAD * latm);

  const double k = 111195.0; // meters/deg
  return (dlat*dlat + dlon*dlon) * k * k;
}

static inline uint32_t every_ms(uint32_t &t, uint32_t period){
  uint32_t now = millis();
  if(now - t >= period){ t = now; return 1; }
  return 0;
}

// -----------------------------------------------------------------------------
// Update_Alfa (LEGACY)
// -----------------------------------------------------------------------------
float Alfa_speed::Update_Alfa(const GPS_speed& M)
{
  if(alfa_counter == 0)
    return alfa_speed_max;

  static int old_run = -1;
  static uint32_t tBeat = 0;

  // ---------------------------------------------------------------------------
  // Heartbeat: prove alpha is alive
  // ---------------------------------------------------------------------------
  if(millis() - tBeat > 1000){
    tBeat = millis();
    Serial.printf(
      "[ALFA] tick idx=%d run=%d alfa=%d M.idx=%d M.samp=%d spd_alfa=%.2f\n",
      index_GPS, run_count, alfa_counter,
      M.m_index, M.m_sample, (float)M.m_speed_alfa
    );
  }

  const int exit  = index_GPS;
  const int entry = M.m_index + 1;

  // ---------------------------------------------------------------------------
  // Geometry + speed eligibility
  // ---------------------------------------------------------------------------
  if(entry >= 0 && exit > entry && M.m_speed_alfa > 0.0f)
  {
    const int entryA = entry % BUFFER_ALFA;
    const int exitA  = exit  % BUFFER_ALFA;

    const double d2 = closure_dist2(entryA, exitA);

    if(d2 < alfa_circle_square && M.m_sample < BUFFER_ALFA)
    {
      const float speed = (float)M.m_speed_alfa;

      Serial.printf(
        "[ALFA] geom OK entry=%d exit=%d closure=%.1fm spd=%.2f\n",
        entry, exit, sqrt(d2), speed
      );

      if(speed > alfa_speed_max)
      {
        alfa_speed_max = speed;
        alfa_speed     = speed;

        real_distance[0] = (int)(sqrt(d2) + 0.5);

        getLocalTime(&tmstruct, 0);
        time_hour[0] = tmstruct.tm_hour;
        time_min [0] = tmstruct.tm_min;
        time_sec [0] = tmstruct.tm_sec;

        this_run[0]      = alfa_counter;
        avg_speed[0]     = alfa_speed_max;
        message_nr[0]    = nav_pvt_message;
        alfa_distance[0] = (int)(M.m_distance_alfa / systemInfo.sample_rate);

        Serial.printf(
          "[ALFA] NEW BEST alfa=%d closure=%dm best=%.2fkn dist=%dm\n",
          alfa_counter,
          real_distance[0],
          alfa_speed_max,
          alfa_distance[0]
        );
      }
    }
  }

  // ---------------------------------------------------------------------------
  // FINALISE ON RUN CHANGE (CORRECT + RP6-COMPATIBLE)
  // ---------------------------------------------------------------------------
  if(run_count != old_run)
  {
    if(old_run >= 0 && alfa_speed_max > 0.0f)
    {
      Serial.printf(
        "[ALFA] FINAL run=%d best=%.2fkn\n",
        old_run, alfa_speed_max
      );

      sort_run_results(
        avg_speed,
        real_distance,
        message_nr,
        time_hour,
        time_min,
        time_sec,
        this_run,
        alfa_distance,
        10
      );
    }

    // Reset for next run
    alfa_speed     = 0.0f;
    alfa_speed_max = 0.0f;
  }

  old_run = run_count;

  display_max_speed =
    (alfa_speed_max > avg_speed[9]) ? alfa_speed_max : avg_speed[9];

  return alfa_speed_max;
}


// -----------------------------------------------------------------------------
// Reset
// -----------------------------------------------------------------------------
void Alfa_speed::Reset_stats()
{
  for(int i=0;i<10;i++) avg_speed[i] = 0.0f;
  alfa_speed     = 0.0f;
  alfa_speed_max = 0.0f;
  Serial.println("[ALFA] RESET");
}
