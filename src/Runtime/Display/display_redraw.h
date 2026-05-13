#pragma once

// ============================================================================
// Display redraw signalling
//
// Thread-safe API used by other modules to request a full or partial e-paper
// redraw. The display task owns the actual rendering and coalesces requests.
// ============================================================================

#include "Display/display_geometry.h"

void screen_request_redraw();

void screen_request_partial(DisplayWindow window);
