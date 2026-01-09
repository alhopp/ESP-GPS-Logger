#pragma once

#include <stdint.h>

void gps_simulator_init();
int  gps_simulator_step();   // returns MT_NAV_PVT when updated
