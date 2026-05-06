#pragma once

// Normalized GPS fix view used by the rest of the firmware.
//
// UBX NAV-PVT is a binary protocol struct with u-blox units. GpsFix converts
// the fields application code needs into clearer units and flags.

#include <Arduino.h>
#include <time.h>

#include "GPS/gps_config.h"
#include "GPS/Ublox/ublox_driver.h"

struct GpsFix {
  bool validFix = false;
  bool validDateTime = false;
  uint8_t satellites = 0;
  uint8_t fixType = 0;
  double lat = 0.0;
  double lon = 0.0;
  float speedKnots = 0.0f;
  float headingDeg = 0.0f;
  float speedMmps = 0.0f;
  float speedAccuracy = 0.0f;
  uint32_t iTow = 0;
  tm utc = {};
};

inline GpsFix gps_fix_from_ubx()
{
  const auto& p = ubxMessage.navPvt;

  GpsFix fix;
  fix.validFix = p.fixType >= 3;
  fix.validDateTime = (p.valid & 0b011) == 0b011;
  fix.satellites = p.numSV;
  fix.fixType = p.fixType;
  fix.lat = p.lat * 1e-7;
  fix.lon = p.lon * 1e-7;
  fix.speedMmps = static_cast<float>(p.gSpeed);
  fix.speedKnots = fix.speedMmps * MMPS_TO_KNOTS;
  fix.headingDeg = p.heading / 100000.0f;
  fix.speedAccuracy = p.sAcc / 1000.0f;
  fix.iTow = p.iTOW;

  fix.utc.tm_year = p.year - 1900;
  fix.utc.tm_mon = p.month - 1;
  fix.utc.tm_mday = p.day;
  fix.utc.tm_hour = p.hour;
  fix.utc.tm_min = p.min;
  fix.utc.tm_sec = p.sec;

  return fix;
}
