#pragma once

// ============================================================================
// screen_speed.h
//
// Speed screen rendering interface.
//
// Responsibilities:
// - Declare SPEED screen draw entry point
// - Declare shared UI state used by speed rendering
//
// Design rules:
// - No logic in this header
// - No display paging or refresh control
// - screen_speed.cpp owns implementation details
// ============================================================================


// -----------------------------------------------------------------------------
// Public draw entry point
// -----------------------------------------------------------------------------
void draw_SPEED();





