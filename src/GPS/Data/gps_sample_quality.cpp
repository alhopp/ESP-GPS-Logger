#include "GPS/Data/gps_sample_quality.h"

// Keeps filtering rules separate from gps_data's buffer ownership.
// Tune bad-point behavior here without touching the RP6-style metric math.

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

float distanceMeters(float lat0, float lon0, float lat1, float lon1)
{
  const float dlat = lat1 - lat0;
  const float dlon = (lon1 - lon0) * cosf((lat0 + lat1) * 0.5f * DEG2RAD);
  return sqrtf(dlat * dlat + dlon * dlon) * 111195.0f;
}

bool sampleQualityOk(float latitude, float longitude, uint32_t gSpeed)
{
  if (ubxMessage.navPvt.fixType < 3) return false;
  if (ubxMessage.navPvt.numSV < FILTER_MIN_SATS) return false;
  if ((ubxMessage.navPvt.sAcc * 0.001f) >= FILTER_MAX_sACC) return false;
  if (gSpeed > (uint32_t)MAX_GPS_SPEED_OK * 1000U) return false;
  if (fabsf(latitude) < MIN_VALID_COORD && fabsf(longitude) < MIN_VALID_COORD) return false;

  if (have_last_good_position) {
    const float sr = systemInfo.sample_rate > 0 ? (float)systemInfo.sample_rate : 5.0f;
    const float expected_m = (float)gSpeed * 0.001f / sr;
    const float max_jump_m = fmaxf(BAD_JUMP_MIN_M, expected_m * BAD_JUMP_SPEED_MULT + BAD_JUMP_MARGIN_M);

    if (distanceMeters(last_good_lat, last_good_lon, latitude, longitude) > max_jump_m) {
      return false;
    }
  }

  return true;
}
} // namespace

GpsSampleQualityResult gps_sample_quality_filter(float latitude,
                                                 float longitude,
                                                 uint32_t gSpeed)
{
  const bool good = sampleQualityOk(latitude, longitude, gSpeed);

  if (!good) {
    if (have_last_good_position) {
      latitude = last_good_lat;
      longitude = last_good_lon;
    }
    gSpeed = 0;
  } else {
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
