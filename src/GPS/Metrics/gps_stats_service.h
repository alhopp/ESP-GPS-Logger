#pragma once

#include "GPS/gps_fix.h"

// Updates all GPS-derived session statistics for one accepted NAV-PVT sample.
void gps_stats_update(const GpsFix& fix);

// Reset service-local cached state at the start of a new logging session.
void gps_stats_service_reset();
