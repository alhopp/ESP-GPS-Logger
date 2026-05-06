#pragma once

// ============================================================================
// gps_fix.h
//
// Normalized GPS fix view used by the rest of the firmware.
//
// UBX NAV-PVT is a binary protocol struct with u-blox units. GpsFix converts
// the fields application code needs into clearer units and flags.
// ============================================================================

#include <Arduino.h>
#include <time.h>

#include "GPS/gps_config.h"
#include "GPS/Ublox/ublox_driver.h"

struct GpsFix {
  bool validFix = false;       // 3D fix or better.
  bool validDateTime = false;  // Receiver says UTC date and time are valid.
  uint8_t satellites = 0;      // NAV-PVT numSV.
  uint8_t fixType = 0;         // Raw u-blox fix type.
  double lat = 0.0;            // Decimal degrees.
  double lon = 0.0;            // Decimal degrees.
  float speedKnots = 0.0f;     // Doppler ground speed converted for display.
  float headingDeg = 0.0f;     // Course over ground, degrees.
  float speedMmps = 0.0f;      // Doppler ground speed, mm/s.
  float speedAccuracy = 0.0f;  // Speed accuracy estimate, m/s.
  uint32_t iTow = 0;           // GPS time of week, ms.
  tm utc = {};                 // UTC calendar time, ready for time APIs.
};

inline GpsFix gps_fix_from_ubx()
{
  const auto& p = ubxMessage.navPvt;

  GpsFix fix;
  fix.validFix = p.fixType >= 3;
  fix.validDateTime = (p.valid & 0b011) == 0b011;
  fix.satellites = p.numSV;
  fix.fixType = p.fixType;

  // u-blox NAV-PVT stores coordinates as degrees * 1e7.
  fix.lat = p.lat * 1e-7;
  fix.lon = p.lon * 1e-7;

  // p.gSpeed is Doppler ground speed in mm/s; keep that canonical value and
  // derive knots only for display/status code.
  fix.speedMmps = static_cast<float>(p.gSpeed);
  fix.speedKnots = fix.speedMmps * MMPS_TO_KNOTS;

  // heading is degrees * 1e5; sAcc is mm/s.
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
