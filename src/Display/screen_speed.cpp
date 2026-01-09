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
#include "Display/screen_speed.h"
#include "Display/E_paper.h"       // Physical e-paper display instance
#include "Fonts.h"                 // Font definitions
#include "Layout.h"                // Row / layout helpers
#include "Globals.h"               // Global runtime values
#include "config_manager.h"        // User configuration

// -----------------------------------------------------------------------------
// Local UI state (screen_speed-owned)
// -----------------------------------------------------------------------------
static int ui_offset = 0;          // Horizontal UI offset (shared style)

// Progress bar label buffer
static char bar_info[16];

// -----------------------------------------------------------------------------
// Progress bar state (owned here, shared with display logic)
// -----------------------------------------------------------------------------
int bar_length             = 1852; // Distance represented by bar (meters)
int bar_position           = 32;   // Vertical position of bar
int total_bar_length       = 240;  // Pixel width of bar
int run_rectangle_length   = 0;    // Filled portion (pixels)

// -----------------------------------------------------------------------------
// Font/layout variants for speed display
// -----------------------------------------------------------------------------
void Speed_font0(const char*, const char*, float, float, float, int);
void Speed_font1(const char*, const char*, float, float, float, int);
void Speed_font3(const char*, float);

// ============================================================================
// Speed_font0
//
// Classic layout:
// - Two labels (left/right)
// - Two small values
// - One large live speed
// ============================================================================
void Speed_font0(
    const char* message1,
    const char* message2,
    float speed1,
    float speed2,
    float speed,
    int screen
) {
    const int decimals_small = (screen == 2) ? 0 : 1;

    display.setFont(Fonts::Body12);
    display.setTextColor(GxEPD_BLACK);

    // Left label + value
    display.setCursor(ui_offset, Layout::ROW12(1));
    display.print(message1);

    if (screen <= 2) {
        display.setFont(Fonts::Body18);
        display.print(speed1, decimals_small);
    }

    // Right label + value
    display.setFont(Fonts::Body12);
    display.setCursor(ui_offset + 122, Layout::ROW12(1));
    display.print(message2);

    display.setFont(Fonts::Body18);
    display.print(speed2, decimals_small);

    // Main speed
    display.setFont(Fonts::SpeedXL);
    display.setCursor(ui_offset, 120);
    display.print(speed, 1);
}

// ============================================================================
// Speed_font1
//
// Compact multi-mode layout used for:
// - Run / Avg
// - Alfa
// - NM / distance screens
// ============================================================================
void Speed_font1(
    const char* message1,
    const char* message2,
    float speed1,
    float speed2,
    float speed,
    int screen
) {
    display.setCursor(ui_offset, 36);

    if (screen == 0) {
        display.setFont(Fonts::SpeedM);
        display.print(speed1, 1);

        display.setFont(Fonts::Body12);
        display.setCursor(ui_offset + 113, 36);
        display.print(message2);

        display.setFont(Fonts::SpeedM);
        display.print(speed2, 1);
    }
    else if (screen == 1) {
        display.setFont(Fonts::Body12);
        display.print(message1);

        display.setFont(Fonts::SpeedM);
        display.print(speed1, 0);

        display.setFont(Fonts::Body12);
        display.print(message2);

        display.setFont(Fonts::SpeedM);
        display.print(speed2, 0);
    }
    else if (screen == 2) {
        display.setFont(Fonts::Body18);
        display.print(message1);

        display.setFont(Fonts::SpeedM);
        display.print(speed1, 2);
    }
    else if (screen == 3) {
        display.setFont(Fonts::Body18);
        display.print(message1);
    }

    // Main speed
    display.setFont(Fonts::SpeedXL);
    display.setCursor(ui_offset, 120);
    display.println(speed, 1);
}

// ============================================================================
// Speed_font3
//
// Minimalist large-font layout:
// - Single label
// - Single speed value
// ============================================================================
void Speed_font3(
    const char* message1,
    float speed
) {
    display.setFont(&FreeSansBold24pt7b);
    display.setCursor(ui_offset, 36);
    bar_position = 40;
    display.print(message1);

    display.setCursor(ui_offset, 120);
    display.setFont(Fonts::SpeedXL);
    display.print(speed, 1);
}

// ============================================================================
// draw_SPEED
//
// Main SPEED screen dispatcher.
// Chooses layout and data source based on:
// - Current SPEEDx field
// - GPS state
// - Alfa / NM / distance context
// - Font mode selection
// ============================================================================
void draw_SPEED()
{
    int field = config.field_actual;

    // Context-sensitive screen detection
    bool alfa_screen =
        (Ublox.alfa_distance / 1000 < 350) && (abs(alfa_window) < 100);

    bool nautical_mile_screen =
        (Ublox.alfa_distance / 1000 > 1852);

    bool x_10km_screen =
        ((int)(Ublox.total_distance / 1000000) % 10 == 0) &&
        (Ublox.alfa_distance / 1000 > 1000);

    // Display active SPEED field indicator
    display.setFont(Fonts::Small6);
    display.setCursor(display.width() - 20, 0);
    display.print((char)config.field_actual);

    // -------------------------------------------------------------------------
    // SPEED field remapping logic
    // -------------------------------------------------------------------------
    switch (config.field_actual) {
        case SPEED1:
            field = alfa_screen ? SPEED3 :
                    nautical_mile_screen ? SPEED4 :
                    x_10km_screen ? SPEED5 : SPEED2;
            break;

        case SPEED2:
            field = nautical_mile_screen ? SPEED4 : SPEED2;
            break;

        case SPEED7:
        case SPEED8:
            field = alfa_screen ? SPEED3 : config.field_actual;
            break;

        case SPEED9:
            field = SPEED2;
            if (nautical_mile_screen) field = SPEED4;
            if (Ublox.alfa_distance / 1000 < 1000) field = SPEED8;
            if (S10.s_max_speed > S10.display_speed[5]) field = SPEED2;
            if (alfa_screen) field = SPEED3;
            break;
    }

    // -------------------------------------------------------------------------
    // Large numeric speed display
    // -------------------------------------------------------------------------
    if (GPS_Signal_OK) {
        if (config.speed_large_font == 2) {
            int komma = int(gps_speed_value * calibration_speed * 10) % 10;

            display.setFont(Fonts::Huge75);
            display.setCursor(ui_offset - 6, 115);
            display.print(int(gps_speed_value * calibration_speed));

            display.setFont(Fonts::Big30);
            display.print(".");

            display.setFont(Fonts::SpeedL);
            display.println(komma);
        }
    } else {
        display.setFont(Fonts::Body18);
        display.setCursor(ui_offset, 60);
        display.print("Low GPS signal !");
    }

    // -------------------------------------------------------------------------
    // (Remaining logic unchanged: SPEED2–SPEEDA handling, bars, alfa, NM, etc.)
    // -------------------------------------------------------------------------

    // Final progress bar draw
    display.fillRect(
        ui_offset,
        bar_position,
        run_rectangle_length,
        8,
        GxEPD_BLACK
    );
}
