#include "GPS/gps_run.h"

#include <math.h>

#include "Core/Definitions.h"
#include "core/system_info.h"
#include "Core/Globals.h"
#include "Core/rtc_state.h"

#include "GPS/GPS_data.h"
#include "GPS/gps_geometry.h"

// -----------------------------------------------------------------------------
// Absolute GPS sample index
// -----------------------------------------------------------------------------
extern int index_GPS;

// Shared position buffers
extern float _lat[BUFFER_ALFA];
extern float _long[BUFFER_ALFA];

// -----------------------------------------------------------------------------
// Global run / alpha markers (DEFINED HERE)
// -----------------------------------------------------------------------------
volatile int alpha_gybe_index      = -1;
volatile int alpha_run_start_index = -1;

// ============================================================================
// New_run_detection (RP6 logic, KNOTS)
// ============================================================================
int New_run_detection(float actual_heading, float speed_kn)
{
  // RP6 thresholds (converted from mm/s)
  const float SPEED_DETECTION_MIN       = 7.5f;  // ≈ 4 m/s
  const float STANDSTILL_DETECTION_MAX  = 2.0f;  // ≈ 1 m/s
  const int   MEAN_HEADING_TIME         = 15;    // seconds
  const float STRAIGHT_COURSE_MAX_DEV   = 10.0f;
  const float JIBE_COURSE_DEVIATION_MIN = 50.0f;

  static float old_heading   = 0.0f;
  static float delta_heading = 0.0f;
  static float heading       = 0.0f;

  static uint32_t delay_counter = 0;
  static int run_counter        = 0;

  static bool velocity_0 = false;
  static bool velocity_5 = false;
  static bool straight_course = false;

  // ---------------------------------------------------------------------------
  // Heading unwrap
  // ---------------------------------------------------------------------------
  if((actual_heading - old_heading) > 300.0f)  delta_heading -= 360.0f;
  if((actual_heading - old_heading) < -300.0f) delta_heading += 360.0f;

  old_heading = actual_heading;
  heading     = actual_heading + delta_heading;
  heading_SD  = heading;

  // ---------------------------------------------------------------------------
  // Mean heading (sliding average)
  // ---------------------------------------------------------------------------
  const float N = MEAN_HEADING_TIME * systemInfo.sample_rate;
  Mean_heading = Mean_heading * (N - 1.0f) / N + heading / N;

  // ---------------------------------------------------------------------------
  // Speed gating
  // ---------------------------------------------------------------------------
  if(speed_kn > SPEED_DETECTION_MIN) velocity_5 = true;
  if(speed_kn < STANDSTILL_DETECTION_MAX && velocity_5) velocity_0 = true;

  if(velocity_0 && speed_kn > SPEED_DETECTION_MIN){
    velocity_0 = false;
    velocity_5 = false;
    delay_counter = (TIME_DELAY_NEW_RUN - 1) * systemInfo.sample_rate;
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
  // Gybe detected → ALPHA boundary
  // ---------------------------------------------------------------------------
  if(fabsf(Mean_heading - heading) > JIBE_COURSE_DEVIATION_MIN &&
     straight_course)
  {
    straight_course = false;
    delay_counter   = 0;

    alfa_counter++;
    alpha_gybe_index = index_GPS;
  }

  // ---------------------------------------------------------------------------
  // Run transition (delayed)
  // ---------------------------------------------------------------------------
  delay_counter++;

  if(delay_counter == TIME_DELAY_NEW_RUN * systemInfo.sample_rate){
    run_counter++;
    alpha_run_start_index = index_GPS;
  }

  return run_counter;
}

// ============================================================================
// Alpha 500 (RP6-style, KNOTS, simplified)
// ============================================================================
//
// Rules enforced:
// - Must STRADDLE a gybe
// - ≤ 500 m sailed
// - ≤ 50 m closure
// - Speed from SECOND leg (GPS_speed::m_speed_alfa)
// ============================================================================

static float alpha_best_kn      = 0.0f;
static int   last_alfa_counter  = -1;

float Alpha500_Update(const GPS_speed& M500)
{
  // No gybe yet
  if(alpha_gybe_index < 0)
    return alpha_best_kn;

  // Reset on new gybe
  if(alfa_counter != last_alfa_counter){
    alpha_best_kn     = 0.0f;
    last_alfa_counter = alfa_counter;
  }

  // Need valid samples
  if(M500.m_sample <= 0)
    return alpha_best_kn;

  // Entry = gybe position
  const int i0 = alpha_gybe_index % BUFFER_ALFA;
  const int i1 = index_GPS % BUFFER_ALFA;

  const float lat0 = _lat[i0];
  const float lon0 = _long[i0];
  const float lat1 = _lat[i1];
  const float lon1 = _long[i1];

  // Straight-line closure distance (meters)
  const float closure_m = afstandPunten(lon0, lat0, lon1, lat1);

  // Must return within 50 m
  if(closure_m > 50.0f)
    return alpha_best_kn;

  // Distance sailed since gybe (mm → m)
  const float sailed_m =
    (float)M500.m_distance_alfa / systemInfo.sample_rate;

  if(sailed_m > 500.0f)
    return alpha_best_kn;

  // Speed from SECOND leg (already knots)
  const float candidate_kn = M500.m_speed_alfa;

  if(candidate_kn > alpha_best_kn){
    alpha_best_kn = candidate_kn;
    RTC_alp_knots = alpha_best_kn;   // snapshot for UI / GeoJSON
  }

  return alpha_best_kn;
}
