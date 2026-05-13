#pragma once

// ============================================================================
// GPS display policy
//
// Converts incoming GPS fixes into redraw requests. Drawing remains in the
// display screen modules; this file only decides when a satellite/status or
// speed refresh is worth requesting.
// ============================================================================

#include "GPS/gps_fix.h"

void gps_display_policy_update(const GpsFix& fix);
