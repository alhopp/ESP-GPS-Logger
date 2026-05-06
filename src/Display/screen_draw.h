#pragma once

// -----------------------------------------------------------------------------
// screen_draw.h
//
// Screen rendering dispatch interface.
//
// Responsibilities:
// - Declare the authoritative mapping from SystemMode to draw function
//
// Design rules:
// - Stateless
// - No rendering logic
// - No mode inference
// - No side effects
//
// Notes:
// - task_display decides when to draw
// - system_mode decides what mode we are in
// -----------------------------------------------------------------------------

#include "Core/system_mode.h"

// Each draw_* function must perform a complete, non-blocking screen render.
using DrawFn = void (*)();

// Return the draw_* function for the given SystemMode, or nullptr if undefined.
DrawFn getDrawFnForMode(SystemMode mode);
