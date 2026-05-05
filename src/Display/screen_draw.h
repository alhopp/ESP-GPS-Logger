#pragma once

// -----------------------------------------------------------------------------
// screen_dispatch.h
//
// Screen rendering dispatch interface.
//
// Responsibilities:
// - Declare the authoritative mapping from SystemMode → draw function
//
// Design rules:
// - Stateless
// - No rendering logic
// - No mode inference
// - No side effects
//
// Notes:
// - task_display decides *when* to draw
// - system_mode decides *what mode we are in*
// -----------------------------------------------------------------------------

#include "Core/system_mode.h"

// -----------------------------------------------------------------------------
// Draw function signature
//
// Each draw_* function must:
// - Perform a complete screen render
// - Be stateless
// - Not block or delay
// -----------------------------------------------------------------------------
using DrawFn = void (*)();

// -----------------------------------------------------------------------------
// MODE → DRAW FUNCTION lookup
//
// Returns:
// - Pointer to draw_* function for the given SystemMode
// - nullptr if no renderer is defined for that mode
// -----------------------------------------------------------------------------
DrawFn getDrawFnForMode(SystemMode mode);
