#include "Display/Screens/screen_system.h"
#include "Display/Screens/screen_system_common.h"

#include "Core/magnet_input.h"
#include "Display/E_paper.h"
#include "Runtime/display_redraw.h"

namespace {
constexpr int MAG_X = 30;
constexpr int MAG_Y = 12;
constexpr int MAG_R = 8;

constexpr DisplayWindow MAGNET_WINDOW = {
  MAG_X - MAG_R - 2,
  MAG_Y - MAG_R - 2,
  MAG_R * 2 + 4,
  MAG_R * 2 + 4
};
} // namespace

void screen_request_magnet_affordance()
{
  screen_request_partial(MAGNET_WINDOW);
}

void drawMagnet()
{
  if (magnet_active) {
    display.fillCircle(MAG_X, MAG_Y, MAG_R, GxEPD_BLACK);
  } else {
    display.drawCircle(MAG_X, MAG_Y, MAG_R, GxEPD_BLACK);
  }
}

void drawCenteredText(const char* text, int y, const GFXfont* font)
{
  int16_t x1;
  int16_t y1;
  uint16_t w;
  uint16_t h;

  display.setFont(font);
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((display.width() - w) / 2, y);
  display.print(text);
}
