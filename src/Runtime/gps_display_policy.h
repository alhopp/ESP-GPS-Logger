#pragma once

#include "GPS/gps_fix.h"

// Owns GPS-driven display refresh policy for the runtime task.
// Screen renderers still own drawing; this module only requests redraw windows.

void gps_display_policy_update(const GpsFix& fix);
