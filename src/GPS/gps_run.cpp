// ============================================================================
// gps_run.cpp
//
// AUTHORITATIVE run + jibe detection
// - Owns run numbering
// - Owns jibe detection
// - Stateless to callers (push model)
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
// Tunables (legacy values preserved)
// -----------------------------------------------------------------------------
static constexpr float SPEED_DETECTION_MIN       = 7.5f;   // kn
static constexpr float STANDSTILL_DETECTION_MAX  = 2.0f;   // kn
static constexpr int   MEAN_HEADING_TIME         = 15;     // s
static constexpr float STRAIGHT_COURSE_MAX_DEV   = 10.0f;  // deg
static constexpr float JIBE_COURSE_DEVIATION_MIN = 50.0f;  // deg

// -----------------------------------------------------------------------------
// Internal persistent state (AUTHORITATIVE)
// -----------------------------------------------------------------------------
static float old_heading   = 0.0f;
static float delta_heading = 0.0f;
static float heading       = 0.0f;


static uint32_t delay_counter = 0;
static int      run_counter   = 0;

static bool velocity_0      = false;
static bool velocity_5      = false;
static bool straight_course = false;

// Per-sample flags
static bool run_started_flag = false;
static bool run_ended_flag   = false;

// Jibe info
static int last_jibe_idx = -1;

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------
void gps_run_update(float heading_deg, float speed_kn)
{
  run_started_flag = false;
  run_ended_flag   = false;

  // ---------------------------------------------------------------------------
  // Heading unwrap (legacy-safe)
  // ---------------------------------------------------------------------------
  if((heading_deg - old_heading) > 300.0f)  delta_heading -= 360.0f;
  if((heading_deg - old_heading) < -300.0f) delta_heading += 360.0f;

  old_heading = heading_deg;
  heading     = heading_deg + delta_heading;
  heading_SD  = heading;

  // ---------------------------------------------------------------------------
  // Mean heading (EWMA over MEAN_HEADING_TIME)
  // ---------------------------------------------------------------------------
  const float N = MEAN_HEADING_TIME * systemInfo.sample_rate;
  Mean_heading = Mean_heading * (N - 1.0f) / N + heading / N;

  // ---------------------------------------------------------------------------
  // Speed gating
  // ---------------------------------------------------------------------------
  if(speed_kn > SPEED_DETECTION_MIN) velocity_5 = true;

  if(speed_kn < STANDSTILL_DETECTION_MAX && velocity_5)
    velocity_0 = true;

  // Restart after standstill → arm delayed new run
  if(velocity_0 && speed_kn > SPEED_DETECTION_MIN){
    velocity_0      = false;
    velocity_5      = false;
    straight_course = false;
    delay_counter   = (TIME_DELAY_NEW_RUN - 1) * systemInfo.sample_rate;
  }

  // ---------------------------------------------------------------------------
  // Straight course detection
  // ---------------------------------------------------------------------------
  if(fabsf(Mean_heading - heading) < STRAIGHT_COURSE_MAX_DEV &&
     speed_kn > SPEED_DETECTION_MIN)
  {
    straight_course = true;
  }

  // ---------------------------------------------------------------------------
  // Jibe detection
  // ---------------------------------------------------------------------------
  if(fabsf(Mean_heading - heading) > JIBE_COURSE_DEVIATION_MIN && straight_course)
  {
    straight_course = false;
    delay_counter   = 0;

    alfa_counter++;
    last_jibe_idx = index_GPS;
  }

  // ---------------------------------------------------------------------------
  // Run counter (delay-based, legacy timing)
  // ---------------------------------------------------------------------------
  delay_counter++;
  if(delay_counter == (uint32_t)(TIME_DELAY_NEW_RUN * systemInfo.sample_rate)){
    run_counter++;
    run_started_flag = true;
  }
}

// -----------------------------------------------------------------------------
// Queries
// -----------------------------------------------------------------------------
int gps_run_current(){ return run_counter; }

bool gps_run_started(){ return run_started_flag; }

bool gps_run_ended(){ return run_ended_flag; } // reserved for future use

int gps_run_last_jibe_index(){ return last_jibe_idx; }
