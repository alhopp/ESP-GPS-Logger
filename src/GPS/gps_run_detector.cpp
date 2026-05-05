// ============================================================================
// gps_run_detector.cpp
//
// AUTHORITATIVE run detection (GPS Speedreader aligned)
//
// KEY BEHAVIOUR (MATCHES SPEEDREADER):
// - Run END occurs EARLY in the turn (mid-gybe, not after completion)
// - Run START requires sustained straight + speed
// - Turn arc accumulation is STICKY across short turn-rate dips
// - No gaps in samples used for 10s / NM / Alpha windows
// ============================================================================

#include "GPS/gps_run_detector.h"
#include <Arduino.h>
#include <math.h>

#include "Core/Definitions.h"
#include "Core/system_info.h"
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
static constexpr float START_MAX_TURN_RATE_DPS  = 4.0f;   // "straight enough"
static constexpr float END_MIN_TURN_RATE_DPS    = 10.0f;  // "committed turn"

// Persistence (samples @ sample_rate)
static constexpr int   START_PERSIST_SAMPLES    = 8;      // ~1.6s @ 5Hz
static constexpr int   END_PERSIST_SAMPLES      = 3;      // ~0.6s @ 5Hz

// Accumulated heading change required to confirm run end
// (Speedreader flips runs mid-turn, not at full 45°)
static constexpr float END_MIN_TURN_ARC_DEG     = 50.0f;

// -----------------------------------------------------------------------------
// Internal persistent state
// -----------------------------------------------------------------------------
static float prev_heading_deg      = 0.0f;
static bool  prev_heading_valid    = false;

static bool  in_run                = false;
static int   run_counter           = 0;

static int   straight_persist      = 0;
static int   turn_persist          = 0;

// Accumulated turn angle (degrees)
static float turn_accum_deg        = 0.0f;

// Debug persistence origins
static int straight_candidate_idx  = -1;
static int turn_candidate_idx      = -1;

// Per-sample flags
static bool run_started_flag       = false;
static bool run_ended_flag         = false;

// Jibe marker
static int last_jibe_idx            = -1;

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
    prev_heading_deg   = heading_deg;
    prev_heading_valid = true;
    return;
  }

  const float sr = (systemInfo.sample_rate > 0)
                 ? (float)systemInfo.sample_rate
                 : 5.0f;

  const float dH = fabsf(angdiff_deg(heading_deg, prev_heading_deg));
  const float turn_rate_dps = dH * sr;

  prev_heading_deg = heading_deg;

  // ---------------------------------------------------------------------------
  // STOP condition (hard reset)
  // ---------------------------------------------------------------------------
  if(speed_kn < SPEED_STOP_MAX){
    straight_persist = 0;
    turn_persist     = 0;
    turn_accum_deg   = 0.0f;

    straight_candidate_idx = -1;
    turn_candidate_idx     = -1;

    if(in_run){
      in_run = false;
      run_ended_flag = true;

      Serial.printf(
        "[RUN END ] #%d @ sample %d  REASON=STOP  spd=%.2f kn  rate=%.1f dps\n",
        run_counter, index_GPS, speed_kn, turn_rate_dps
      );
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
  // Build persistence + accumulate turn angle
  // ---------------------------------------------------------------------------
  if(straight_now){
    if(straight_persist == 0){
      straight_candidate_idx = index_GPS;
      Serial.printf(
        "[STRAIGHT ?] sample=%d  spd=%.2f kn  rate=%.1f dps\n",
        index_GPS, speed_kn, turn_rate_dps
      );
    }
    straight_persist++;
  }else{
    straight_persist = 0;
    straight_candidate_idx = -1;
  }

  if(turning_now){
    if(turn_persist == 0){
      turn_candidate_idx = index_GPS;
      turn_accum_deg = 0.0f;   // reset only at START of committed turn
      Serial.printf(
        "[TURN ?]     sample=%d  spd=%.2f kn  rate=%.1f dps\n",
        index_GPS, speed_kn, turn_rate_dps
      );
    }
    turn_persist++;
    turn_accum_deg += dH;
  }else{
    // IMPORTANT:
    // Do NOT wipe accumulated arc unless we are clearly straight again
    turn_persist = 0;

    if(turn_rate_dps < START_MAX_TURN_RATE_DPS){
      turn_accum_deg = 0.0f;
      turn_candidate_idx = -1;
    }
  }

  // ---------------------------------------------------------------------------
  // RUN START (requires sustained straight)
  // ---------------------------------------------------------------------------
  if(!in_run && straight_persist >= START_PERSIST_SAMPLES){
    in_run = true;
    run_counter++;
    run_started_flag = true;

    turn_persist   = 0;
    turn_accum_deg = 0.0f;

    Serial.printf(
      "[RUN START] #%d @ sample %d  straight_since=%d  spd=%.2f kn  rate=%.1f dps\n",
      run_counter,
      index_GPS,
      straight_candidate_idx,
      speed_kn,
      turn_rate_dps
    );
  }

  // ---------------------------------------------------------------------------
  // RUN END (early in turn — Speedreader behaviour)
  // ---------------------------------------------------------------------------
  if(in_run &&
     turn_persist   >= END_PERSIST_SAMPLES &&
     turn_accum_deg >= END_MIN_TURN_ARC_DEG)
  {
    in_run = false;
    run_ended_flag = true;

    alfa_counter++;
    last_jibe_idx = index_GPS;

    straight_persist = 0;
    turn_persist     = 0;
    turn_accum_deg   = 0.0f;

    Serial.printf(
      "[RUN END ] #%d @ sample %d  turn_since=%d  arc=%.1f°  spd=%.2f kn  rate=%.1f dps\n",
      run_counter,
      index_GPS,
      turn_candidate_idx,
      turn_accum_deg,
      speed_kn,
      turn_rate_dps
    );
  }
}

// -----------------------------------------------------------------------------
// Queries
// -----------------------------------------------------------------------------
int  gps_run_current(){ return run_counter; }
bool gps_run_started(){ return run_started_flag; }
bool gps_run_ended()  { return run_ended_flag; }
int  gps_run_last_jibe_index(){ return last_jibe_idx; }
