#pragma once

#include "GPS/gps_fix.h"

// Updates all GPS-derived session statistics for one accepted NAV-PVT sample.
void gps_stats_update(const GpsFix& fix);

