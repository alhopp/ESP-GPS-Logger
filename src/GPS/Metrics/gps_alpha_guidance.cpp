#include "GPS/Metrics/gps_alpha_guidance.h"

#include <Arduino.h>
#include <math.h>

#include "Config/config_types.h"
#include "Core/Globals.h"
#include "Core/system_info.h"
#include "GPS/Data/gps_data.h"
#include "GPS/Data/gps_runtime_instances.h"
#include "GPS/gps_config.h"
#include "GPS/Metrics/gps_run_detector.h"

namespace {
constexpr float TARGET_CLOSURE_M = 48.0f;
constexpr float HOLD_BAND_M = 0.75f;
constexpr float ACTIVE_DISTANCE_AFTER_GYBE_M = 650.0f;
constexpr float ALPHA_COMPLETE_DISTANCE_M = 500.0f;
constexpr float ALPHA_MISSED_DISTANCE_M = 540.0f;
constexpr float VALID_ALPHA_CLOSURE_M = 50.0f;
constexpr float METERS_PER_DEGREE = 111195.0f;

AlphaGuidanceState state;
int gybe_index = -1;
int reference_start = -1;
int reference_end = -1;

float sampleRate()
{
  return systemInfo.sample_rate > 0 ? static_cast<float>(systemInfo.sample_rate) : 5.0f;
}

int alphaSlot(int gpsIndex)
{
  return gpsIndex % BUFFER_ALFA;
}

bool validIndex(int gpsIndex)
{
  return gpsIndex >= 0 && (index_GPS - gpsIndex) < BUFFER_ALFA;
}

void pointMetersFromReference(int referenceSlot, int pointSlot, float& northM, float& eastM)
{
  const float lat0 = _lat[referenceSlot];
  const float lon0 = _long[referenceSlot];
  const float lat1 = _lat[pointSlot];
  const float lon1 = _long[pointSlot];

  northM = (lat1 - lat0) * METERS_PER_DEGREE;
  eastM = (lon1 - lon0) * cosf(DEG2RAD * lat1) * METERS_PER_DEGREE;
}

float distanceMeters(int a, int b)
{
  float northM = 0.0f;
  float eastM = 0.0f;
  pointMetersFromReference(alphaSlot(a), alphaSlot(b), northM, eastM);
  return sqrtf(northM * northM + eastM * eastM);
}

float signedCrossTrackMeters(int lineStart, int lineEnd, int point)
{
  const int startSlot = alphaSlot(lineStart);
  const int endSlot = alphaSlot(lineEnd);
  const int pointSlot = alphaSlot(point);

  float lineNorthM = 0.0f;
  float lineEastM = 0.0f;
  float pointNorthM = 0.0f;
  float pointEastM = 0.0f;

  pointMetersFromReference(startSlot, endSlot, lineNorthM, lineEastM);
  pointMetersFromReference(startSlot, pointSlot, pointNorthM, pointEastM);

  const float lineLenM = sqrtf(lineNorthM * lineNorthM + lineEastM * lineEastM);
  if (lineLenM < 1.0f) return 0.0f;

  // Positive/negative only indicates which side of the pre-gybe track the rider
  // is on. The display uses the magnitude for "too wide" vs "too tight".
  return (lineEastM * pointNorthM - lineNorthM * pointEastM) / lineLenM;
}

float alphaPathMeters()
{
  return static_cast<float>(speed_500m.m_distance_alfa) / sampleRate() / 1000.0f;
}

AlphaSteerAdvice adviceFor(float closureM)
{
  const float error = closureM - TARGET_CLOSURE_M;
  if (fabsf(error) <= HOLD_BAND_M) return AlphaSteerAdvice::Hold;

  // Larger closure means the return track is too wide: point higher/upwind to
  // close the alpha. Smaller closure means the return is too tight: bear away.
  return error > 0.0f ? AlphaSteerAdvice::GoUp : AlphaSteerAdvice::GoDown;
}

void deactivate()
{
  state.active = false;
  state.advice = AlphaSteerAdvice::Inactive;
}

void captureGybeReference()
{
  gybe_index = gps_run_last_jibe_index();
  reference_start = speed_250m.m_index + 1;
  reference_end = speed_100m.m_index + 1;

  if (!validIndex(gybe_index) || !validIndex(reference_start) || !validIndex(reference_end)) {
    deactivate();
    return;
  }

  state.active = true;
  state.targetClosureM = TARGET_CLOSURE_M;
}
} // namespace

void gps_alpha_guidance_reset()
{
  state = {};
  state.targetClosureM = TARGET_CLOSURE_M;
  gybe_index = -1;
  reference_start = -1;
  reference_end = -1;
}

void gps_alpha_guidance_update()
{
  if (!config.stat_alpha) {
    deactivate();
    return;
  }

  if (gps_run_ended()) {
    captureGybeReference();
  }

  if (!state.active) return;

  if (!validIndex(gybe_index) || !validIndex(reference_start) || !validIndex(reference_end)) {
    deactivate();
    return;
  }

  state.pathSinceGybeM = distanceMeters(gybe_index, index_GPS);
  if (state.pathSinceGybeM > ACTIVE_DISTANCE_AFTER_GYBE_M) {
    deactivate();
    return;
  }

  state.closureM = fabsf(signedCrossTrackMeters(reference_start, reference_end, index_GPS));
  state.errorM = state.closureM - TARGET_CLOSURE_M;
  state.pathSinceGybeM = alphaPathMeters();
  state.alphaSpeedKnots = static_cast<float>(speed_500m.m_speed_alfa) * MMPS_TO_KNOTS;
  state.advice = adviceFor(state.closureM);

  const bool alphaComplete =
      state.pathSinceGybeM >= ALPHA_COMPLETE_DISTANCE_M &&
      state.closureM <= VALID_ALPHA_CLOSURE_M &&
      speed_500m.m_speed_alfa > 0.0;

  const bool alphaMissed =
      state.pathSinceGybeM >= ALPHA_MISSED_DISTANCE_M &&
      state.closureM > VALID_ALPHA_CLOSURE_M;

  if (alphaComplete || alphaMissed) {
    deactivate();
  }
}

const AlphaGuidanceState& gps_alpha_guidance_state()
{
  return state;
}
