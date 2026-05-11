// -----------------------------------------------------------------------------
// screen_speed.cpp
//
// Speed screen rendering.
//
// Responsibilities:
// - Render the primary live speed screen
// - Display live speed in knots
//
// Design rules:
// - Stateless rendering only, no paging or refresh control
// - Uses global GPS state as read-only input
// - Display task owns refresh timing
// -----------------------------------------------------------------------------

#include "Display/Screens/screen_speed.h"

#include "Core/Globals.h"
#include "Display/E_paper.h"
#include "Display/Screens/ui_text.h"
#include "GPS/gps_runtime_state.h"
#include "Fonts.h"
#include "GPS/gps_config.h"
#include "Layout.h"
#include "Storage/storage_manager.h"

namespace {
constexpr int UI_OFFSET = 0;
constexpr int SPEED_Y = 112;

void drawSavingSession()
{
  drawCenteredText("ESP-GPS", 20, Fonts::Body12);
  drawCenteredText("Saving session", 50, Fonts::Body12);
  drawCenteredText("Building map file", 78, Fonts::Body9);
  drawCenteredText("Please wait", 102, Fonts::Body9);
}
}

void draw_SPEED()
{
  if (storage_is_shutting_down()) {
    drawSavingSession();
    return;
  }

  if (!GPS_Signal_OK) {
    display.setFont(Fonts::Body12);
    display.setCursor(UI_OFFSET, 60);
    display.print("Low GPS signal");
    return;
  }

  const float speedKnots = gps_speed_value * MMPS_TO_KNOTS;
  const int whole = static_cast<int>(speedKnots);
  const int frac = static_cast<int>(speedKnots * 10) % 10;

  display.setFont(Fonts::Huge75);
  display.setCursor(UI_OFFSET + 8, SPEED_Y);
  display.print(whole);

  display.setFont(Fonts::Big30);
  display.print(".");

  display.setFont(Fonts::SpeedL);
  display.print(frac);
}
