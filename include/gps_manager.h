#pragma once

enum class GpsChip {
  UNKNOWN,
  UBLOX_M8,
  UBLOX_M9,
  UBLOX_M10
};

enum class M10NavMode {
  NAV_DEFAULT,
  NAV_HIGH_RATE
};

void initGPS();
void Ublox_on();
void Ublox_off();   
GpsChip getGpsChip();
