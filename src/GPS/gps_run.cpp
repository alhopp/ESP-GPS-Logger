// ============================================================================
// gps_run.cpp
//
// LEGACY Run + jibe detection
//
// RESPONSIBILITY:
// - Detect new runs (standstill + delay)
// - Detect jibes (heading deviation from mean while straight-course locked)
// - Increment alfa_counter on jibe (legacy alpha trigger)
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

// ============================================================================
// New_run_detection  (legacy behaviour)
// ============================================================================
// Inputs:
// - actual_heading : degrees (can be unwrapped negative, legacy-safe)
// - speed_kn       : knots
int New_run_detection(float actual_heading, float speed_kn)
{
  const float SPEED_DETECTION_MIN       = 7.5f;   // kn
  const float STANDSTILL_DETECTION_MAX  = 2.0f;   // kn
  const int   MEAN_HEADING_TIME         = 15;     // seconds
  const float STRAIGHT_COURSE_MAX_DEV   = 10.0f;  // deg
  const float JIBE_COURSE_DEVIATION_MIN = 50.0f;  // deg

  static float old_heading   = 0.0f;
  static float delta_heading = 0.0f;
  static float heading       = 0.0f;

  static uint32_t delay_counter = 0;
  static int      run_counter   = 0;

  static bool velocity_0      = false;
  static bool velocity_5      = false;
  static bool straight_course = false;

  // ---------------------------------------------------------------------------
  // Heading unwrap (legacy)
  // ---------------------------------------------------------------------------
  if((actual_heading - old_heading) > 300.0f)  delta_heading -= 360.0f;
  if((actual_heading - old_heading) < -300.0f) delta_heading += 360.0f;

  old_heading = actual_heading;
  heading     = actual_heading + delta_heading;
  heading_SD  = heading;

  // ---------------------------------------------------------------------------
  // Mean heading (15s EWMA-ish legacy filter)
  // ---------------------------------------------------------------------------
  const float N = (float)MEAN_HEADING_TIME * (float)systemInfo.sample_rate;
  Mean_heading = Mean_heading * (N - 1.0f) / N + heading / N;

  // ---------------------------------------------------------------------------
  // DEBUG heartbeat (~1 Hz)
  // ---------------------------------------------------------------------------
  static uint32_t lastPrint = 0;
  if(millis() - lastPrint > 1000){
    lastPrint = millis();
    Serial.printf("[RUN] spd=%.2fkn hdg=%.1f mean=%.1f sc=%d v5=%d v0=%d run=%d alfa=%d idx=%d\n",
      speed_kn, heading, Mean_heading,
      (int)straight_course, (int)velocity_5, (int)velocity_0,
      run_counter, alfa_counter, index_GPS
    );
  }

  // ---------------------------------------------------------------------------
  // Speed gating
  // ---------------------------------------------------------------------------
  if(speed_kn > SPEED_DETECTION_MIN){
    if(!velocity_5) Serial.println("[RUN] Speed gate OPEN (velocity_5)");
    velocity_5 = true;
  }

  if(speed_kn < STANDSTILL_DETECTION_MAX && velocity_5){
    if(!velocity_0) Serial.println("[RUN] Standstill detected (velocity_0)");
    velocity_0 = true;
  }

  // Restart after standstill
  if(velocity_0 && speed_kn > SPEED_DETECTION_MIN){
    Serial.println("[RUN] Restart after standstill → new run after delay");
    velocity_0       = false;
    velocity_5       = false;
    straight_course  = false;
    delay_counter    = (TIME_DELAY_NEW_RUN - 1) * systemInfo.sample_rate;
  }

  // ---------------------------------------------------------------------------
  // Straight course detection
  // ---------------------------------------------------------------------------
  if(fabsf(Mean_heading - heading) < STRAIGHT_COURSE_MAX_DEV &&
     speed_kn > SPEED_DETECTION_MIN)
  {
    if(!straight_course) Serial.println("[RUN] Straight course LOCKED");
    straight_course = true;
  }

  // ---------------------------------------------------------------------------
  // Jibe detection (legacy)
  // ---------------------------------------------------------------------------
  if(fabsf(Mean_heading - heading) > JIBE_COURSE_DEVIATION_MIN && straight_course)
  {
    straight_course = false;
    delay_counter   = 0;

    alfa_counter++; // legacy alpha trigger
    Serial.printf("[RUN] >>> JIBE DETECTED (alfa=%d) <<<\n", alfa_counter);
  }

  // ---------------------------------------------------------------------------
  // Run counter (delay based)
  // ---------------------------------------------------------------------------
  delay_counter++;
  if(delay_counter == (uint32_t)(TIME_DELAY_NEW_RUN * systemInfo.sample_rate)){
    run_counter++;
    Serial.printf("[RUN] *** NEW RUN %d ***\n", run_counter);
  }

  return run_counter;
}
