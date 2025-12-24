#pragma once
#include <stdint.h>

bool     initGPS();
bool     gps_is_ok();
uint32_t gps_get_baud();
void     gps_shutdown();
void     gps_debug_dump(uint32_t ms);
