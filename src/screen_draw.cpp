#include "screen_draw.h"
#include "Fonts.h"
#include "Layout.h"
#include "E_paper.h"

const DrawFn ScreenDrawTable[] = {
  draw_BOOT,
  draw_GPS_INIT,

  draw_WIFI_ON,
  draw_WIFI_STATION,
  draw_WIFI_SOFT_AP,

  draw_SPEED,

  draw_STATS1,
  draw_STATS2,
  draw_STATS3,
  draw_STATS4,
  draw_STATS5,
  draw_STATS6,
  draw_STATS7,
  draw_STATS8,
  draw_STATS9,
  draw_STATSA,
  draw_STATSB
};

void drawTopLeftTitle(const char* msg)
{
    display.setFont(Fonts::Body9);
    display.print(msg);
}

