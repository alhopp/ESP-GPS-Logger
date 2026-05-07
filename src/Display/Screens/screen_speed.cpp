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
#include "GPS/Metrics/gps_alpha_guidance.h"
#include "Layout.h"
#include "Storage/storage_manager.h"

namespace {
constexpr int UI_OFFSET = 0;
constexpr int SPEED_Y = 112;

const char* adviceText(AlphaSteerAdvice advice)
{
  switch (advice) {
    case AlphaSteerAdvice::GoUp:   return "UP";
    case AlphaSteerAdvice::GoDown: return "DOWN";
    case AlphaSteerAdvice::Hold:   return "GOOD";
    default:                       return "";
  }
}

void drawAlphaHelper()
{
  const AlphaGuidanceState& helper = gps_alpha_guidance_state();

  display.setFont(Fonts::Body12);
  display.setCursor(UI_OFFSET + 4, 18);
  display.print("ALPHA HELPER");

  display.setFont(Fonts::Body18);
  display.setCursor(UI_OFFSET + 8, 58);
  display.print(adviceText(helper.advice));

  display.setFont(Fonts::Body12);
  display.setCursor(UI_OFFSET + 8, 88);
  display.print("Closure ");
  display.print(helper.closureM, 0);
  display.print("m / ");
  display.print(helper.targetClosureM, 0);
  display.print("m");

  display.setCursor(UI_OFFSET + 8, 108);
  display.print("Alpha ");
  display.print(helper.alphaSpeedKnots, 1);
  display.print(" kt");
}

void drawSavingSession()
{
  drawCenteredText("ESP-GPS", Layout::ROW9(2), Fonts::Body12);
  drawCenteredText("Saving session", Layout::ROW9(4), Fonts::Body12);
  drawCenteredText("Building map file", Layout::ROW9(6), Fonts::Body9);
  drawCenteredText("Please wait", Layout::ROW9(8), Fonts::Body9);
}
}

void draw_SPEED()
{
  if (storage_is_shutting_down()) {
    drawSavingSession();
    return;
  }

  if (gps_alpha_guidance_state().active) {
    drawAlphaHelper();
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
