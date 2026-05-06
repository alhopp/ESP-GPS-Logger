#pragma once

#include <stdint.h>

// Compile-time GPS simulator used when GPS_SIMULATOR is enabled.
// It fills the same UBX NAV-PVT globals as the real parser, so downstream GPS
// statistics and logging code exercise the normal path.

void gps_simulator_init();
int  gps_simulator_step();   // returns MT_NAV_PVT when updated
