// -----------------------------------------------------------------------------
// screen_speed.cpp
//
// Speed screen rendering.
//
// Responsibilities:
// - Render all speed-related UI screens
// - Select appropriate layout based on active SPEED mode
// - Display live speed, averages, run stats, alfa stats, and progress bars
//
// Design rules:
// - Stateless rendering only (no paging, no refresh control)
// - Uses global GPS + config state as read-only inputs
// - Display task owns refresh timing
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Display + UI dependencies
// -----------------------------------------------------------------------------
#include "Display/Screens/screen_speed.h"
#include "Display/E_paper.h"       // Physical e-paper display instance
#include "Fonts.h"                 // Font definitions
#include "Core/Globals.h"               // Global runtime values
#include "GPS/gps_config.h"

// -----------------------------------------------------------------------------
// Local UI state Layout constants
// -----------------------------------------------------------------------------
static int ui_offset  = 0;          // Horizontal UI offset (shared style)
constexpr int SPEED_Y = 112;        // Vertical UI offset

// ============================================================================
// draw_SPEED
//
// Primary SPEED screen.
// - Shows large numeric speed (knots)
// - Displays "Low GPS signal" if GPS not valid
// - NO alfa / NM / distance logic here
// ============================================================================

void draw_SPEED()
{
    // -------------------------------------------------------------------------
    // GPS not good enough → warning
    // -------------------------------------------------------------------------
    if (!GPS_Signal_OK) {
        display.setFont(Fonts::Body12);
        display.setCursor(ui_offset, 60);
        display.print("Low GPS signal");
        return;
    }

    // -------------------------------------------------------------------------
    // Speed in knots (raw GPS → knots)
    // gps_speed_value is mm/s
    // -------------------------------------------------------------------------

    const float speed_knots = gps_speed_value * MMPS_TO_KNOTS;
    const int whole = int(speed_knots);
    const int frac  = int(speed_knots * 10) % 10;

    // -------------------------------------------------------------------------
    // Draw large speed
    // -------------------------------------------------------------------------
    display.setFont(Fonts::Huge75);
    display.setCursor(ui_offset + 8, SPEED_Y);
    display.print(whole);

    display.setFont(Fonts::Big30);
    display.print(".");

    display.setFont(Fonts::SpeedL);
    display.print(frac);
}
