#pragma once

#include "Display/display_geometry.h"

// Request a full screen redraw (async, display-task owned)
void screen_request_redraw();

// Request a partial screen redraw (async, display-task owned)
void screen_request_partial(DisplayWindow window);
