#include "GPS/Data/gps_sample_quality.h"

// ============================================================================
// gps_sample_quality.cpp
//
// Keeps bad-point filtering separate from gps_data's ring-buffer ownership.
// This lets us tune GPS trust rules without touching the RP6-style metric math.
// ============================================================================

#include <math.h>

#include "Core/system_info.h"
#include "GPS/Data/gps_data.h"
#include "GPS/gps_config.h"
#include "GPS/Ublox/ublox_driver.h"

namespace {
constexpr float MIN_VALID_COORD = 0.000001f;
constexpr float BAD_JUMP_MIN_M = 50.0f;
constexpr float BAD_JUMP_MARGIN_M = 20.0f;
constexpr float BAD_JUMP_SPEED_MULT = 3.0f;

bool have_last_good_position = false;
float last_good_lat = 0.0f;
float last_good_lon = 0.0f;

// Local flat-earth distance approximation. It is accurate enough for the short
// jumps we are rejecting here and avoids heavier haversine math per GPS sample.
float distanceMeters(float lat0, float lon0, float lat1, float lon1)
{
  const float dlat = lat1 - lat0;
  const float dlon = (lon1 - lon0) * cosf((lat0 + lat1) * 0.5f * DEG2RAD);
  return sqrtf(dlat * dlat + dlon * dlon) * 111195.0f;
}

float sampleRateHz()
{
  return systemInfo.sample_rate > 0 ? (float)systemInfo.sample_rate : 5.0f;
}

bool hasNavigationFix()
{
  return ubxMessage.navPvt.fixType >= 3;
}

bool hasEnoughSatellites()
{
  return ubxMessage.navPvt.numSV >= FILTER_MIN_SATS;
}

bool speedAccuracyOk()
{
  return (ubxMessage.navPvt.sAcc * 0.001f) < FILTER_MAX_sACC;
}

bool speedWithinConfiguredLimit(uint32_t gSpeed)
{
  return gSpeed <= (uint32_t)MAX_GPS_SPEED_OK * 1000U;
}

bool coordinateLooksReal(float latitude, float longitude)
{
  return fabsf(latitude) >= MIN_VALID_COORD || fabsf(longitude) >= MIN_VALID_COORD;
}

bool jumpDistanceOk(float latitude, float longitude, uint32_t gSpeed)
{
  if (!have_last_good_position) return true;

  // Expected travel distance uses the current Doppler speed and sample rate.
  // The multiplier/margin gives enough slack for real acceleration and normal
  // GPS noise while still blocking large one-sample coordinate spikes.
  const float expected_m = (float)gSpeed * 0.001f / sampleRateHz();
  const float max_jump_m =
      fmaxf(BAD_JUMP_MIN_M, expected_m * BAD_JUMP_SPEED_MULT + BAD_JUMP_MARGIN_M);

  return distanceMeters(last_good_lat, last_good_lon, latitude, longitude) <= max_jump_m;
}

bool sampleQualityOk(float latitude, float longitude, uint32_t gSpeed)
{
  return hasNavigationFix() &&
         hasEnoughSatellites() &&
         speedAccuracyOk() &&
         speedWithinConfiguredLimit(gSpeed) &&
         coordinateLooksReal(latitude, longitude) &&
         jumpDistanceOk(latitude, longitude, gSpeed);
}
} // namespace

GpsSampleQualityResult gps_sample_quality_filter(float latitude,
                                                 float longitude,
                                                 uint32_t gSpeed)
{
  const bool good = sampleQualityOk(latitude, longitude, gSpeed);

  if (!good) {
    // Preserve geometry continuity by holding position at the last trusted
    // coordinate, but force speed to zero so the sample cannot improve stats.
    if (have_last_good_position) {
      latitude = last_good_lat;
      longitude = last_good_lon;
    }
    gSpeed = 0;
  } else {
    // Only samples that pass every check become the reference for later jump
    // rejection.
    have_last_good_position = true;
    last_good_lat = latitude;
    last_good_lon = longitude;
  }

  return { good, latitude, longitude, gSpeed };
}

void gps_sample_quality_reset()
{
  have_last_good_position = false;
  last_good_lat = 0.0f;
  last_good_lon = 0.0f;
}
