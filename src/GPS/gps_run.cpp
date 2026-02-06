// ============================================================================
// gps_run.cpp
//
// AUTHORITATIVE run detection (GPS Speedreader aligned)
//
// KEY IDEA (fix for "one run only"):
// - Do NOT use Mean_heading deviation as the primary straight/turn discriminator.
//   After a 190° turn, an EWMA mean lags and can prevent reacquiring "straight".
// - Use TURN RATE (deg/sec) with persistence instead.
//
// Run definition:
// - Run starts when speed is above threshold AND turn-rate stays low (straight)
// - Run ends when turn-rate stays high (committed turn) OR speed drops to stop
//
// DEBUG:
// - Prints run start/end with GPS sample index and turn-rate
//
// Call gps_run_update() once per GPS sample
// ============================================================================

#include "GPS/gps_run.h"
#include <Arduino.h>
#include <math.h>

#include "Core/Definitions.h"
#include "core/system_info.h"
#include "Core/Globals.h"

// -----------------------------------------------------------------------------
// External GPS state
// -----------------------------------------------------------------------------
extern int index_GPS;
extern int alfa_counter;

// -----------------------------------------------------------------------------
// Tunables (Speedreader-ish)
// -----------------------------------------------------------------------------
static constexpr float SPEED_START_MIN          = 7.5f;   // kn
static constexpr float SPEED_STOP_MAX           = 2.0f;   // kn

// Turn-rate thresholds (deg/sec)
static constexpr float START_MAX_TURN_RATE_DPS  = 4.0f;   // "straight enough" to start
static constexpr float END_MIN_TURN_RATE_DPS    = 12.0f;  // "committed turn" to end

// Persistence (samples @ sample_rate)
static constexpr int   START_PERSIST_SAMPLES    = 8;      // ~1.6s @ 5Hz
static constexpr int   END_PERSIST_SAMPLES      = 3;      // ~0.6s @ 5Hz

// -----------------------------------------------------------------------------
// Internal persistent state
// -----------------------------------------------------------------------------
static float prev_heading_deg = 0.0f;
static bool  prev_heading_valid = false;

static bool  in_run = false;
static int   run_counter = 0;

static int   straight_persist = 0;
static int   turn_persist     = 0;

// Per-sample flags
static bool run_started_flag = false;
static bool run_ended_flag   = false;

// Jibe marker
static int last_jibe_idx = -1;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
static inline float angdiff_deg(float a, float b){
  float d = a - b;
  while(d > 180.0f) d -= 360.0f;
  while(d < -180.0f) d += 360.0f;
  return d;
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------
void gps_run_update(float heading_deg, float speed_kn)
{
  run_started_flag = false;
  run_ended_flag   = false;

  // Need a previous heading to compute turn-rate
  if(!prev_heading_valid){
    prev_heading_deg = heading_deg;
    prev_heading_valid = true;
    return;
  }

  const float sr = (systemInfo.sample_rate > 0) ? (float)systemInfo.sample_rate : 5.0f;

  // Turn-rate (deg/sec)
  const float dH = fabsf(angdiff_deg(heading_deg, prev_heading_deg));
  const float turn_rate_dps = dH * sr;

  prev_heading_deg = heading_deg;

  // ---------------------------------------------------------------------------
  // STOP condition
  // ---------------------------------------------------------------------------
  if(speed_kn < SPEED_STOP_MAX){
    straight_persist = 0;
    turn_persist     = 0;

    if(in_run){
      in_run = false;
      run_ended_flag = true;

      Serial.printf("[RUN END ] #%d @ sample %d  REASON=STOP  spd=%.2f kn  rate=%.1f dps\n",
                    run_counter, index_GPS, speed_kn, turn_rate_dps);
    }
    return;
  }

  // ---------------------------------------------------------------------------
  // Classify this sample
  // ---------------------------------------------------------------------------
  const bool straight_now =
    (speed_kn > SPEED_START_MIN) &&
    (turn_rate_dps <= START_MAX_TURN_RATE_DPS);

  const bool turning_now =
    (turn_rate_dps >= END_MIN_TURN_RATE_DPS);

  // ---------------------------------------------------------------------------
  // Build persistence
  // ---------------------------------------------------------------------------
  if(straight_now) straight_persist++; else straight_persist = 0;
  if(turning_now)  turn_persist++;     else turn_persist     = 0;

  // ---------------------------------------------------------------------------
  // RUN START (requires sustained straight)
  // ---------------------------------------------------------------------------
  if(!in_run && straight_persist >= START_PERSIST_SAMPLES){
    in_run = true;
    run_counter++;
    run_started_flag = true;

    // Clear turn persistence so we don't instantly end on noisy boundary
    turn_persist = 0;

    Serial.printf("[RUN START] #%d @ sample %d  spd=%.2f kn  rate=%.1f dps  (persist=%d)\n",
                  run_counter, index_GPS, speed_kn, turn_rate_dps, straight_persist);
  }

  // ---------------------------------------------------------------------------
  // RUN END (requires sustained turning)
  // ---------------------------------------------------------------------------
  if(in_run && turn_persist >= END_PERSIST_SAMPLES){
    in_run = false;
    run_ended_flag = true;

    alfa_counter++;
    last_jibe_idx = index_GPS;

    // Clear straight persistence so we don't instantly start again mid-turn
    straight_persist = 0;
    turn_persist     = 0;

    Serial.printf("[RUN END ] #%d @ sample %d  REASON=TURN  spd=%.2f kn  rate=%.1f dps  (persist=%d)\n",
                  run_counter, index_GPS, speed_kn, turn_rate_dps, END_PERSIST_SAMPLES);
  }
}

// -----------------------------------------------------------------------------
// Queries
// -----------------------------------------------------------------------------
int  gps_run_current(){ return run_counter; }
bool gps_run_started(){ return run_started_flag; }
bool gps_run_ended()  { return run_ended_flag; }
int  gps_run_last_jibe_index(){ return last_jibe_idx; }
