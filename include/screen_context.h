#pragma once

#include "Config.h"
#include "Ublox.h"
#include "GPS_data.h"   // S2, S10, M1852, A500 types

extern float calibration_speed;
extern int   gps_speed;

extern float alfa_window;
extern float alfa_exit;

extern GPS_time  S2;
extern GPS_time  S10;
extern GPS_speed M1852;
extern Alfa_speed A500;

extern Config config;
