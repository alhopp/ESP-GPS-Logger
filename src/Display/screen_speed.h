#pragma once

// ============================================================================
// screen_speed.h
//
// Speed screen rendering interface.
//
// Responsibilities:
// - Declare SPEED screen draw entry point
// - Declare shared UI state used by speed rendering
// - Expose font/layout helpers used internally by screen_speed.cpp
//
// Design rules:
// - No logic in this header
// - No display paging or refresh control
// - screen_speed.cpp owns implementation details
// - task_display controls when drawing occurs
// ============================================================================


// -----------------------------------------------------------------------------
// Public draw entry point
// -----------------------------------------------------------------------------
void draw_SPEED();

// -----------------------------------------------------------------------------
// Shared UI state (owned by screen_speed.cpp)
//
// These are read by other UI components (e.g. chrome, layout helpers)
// and therefore must remain externally visible.
// -----------------------------------------------------------------------------
extern int bar_length;             // Distance represented by progress bar (m)
extern int bar_position;           // Vertical position of progress bar (px)
extern int total_bar_length;       // Total bar width (px)
extern int run_rectangle_length;   // Filled portion of bar (px)

// -----------------------------------------------------------------------------
// Speed font/layout helpers
//
// These are implementation helpers but are exposed for consistency with
// existing screen modules and potential reuse.
// -----------------------------------------------------------------------------
void Speed_font0(const char* message1,
                 const char* message2,
                 float speed1,
                 float speed2,
                 float speed,
                 int screen);

void Speed_font1(const char* message1,
                 const char* message2,
                 float speed1,
                 float speed2,
                 float speed,
                 int screen);

void Speed_font3(const char* message,
                 float speed);
