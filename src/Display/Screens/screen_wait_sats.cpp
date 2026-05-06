#include "Display/Screens/screen_system.h"

#include <Arduino.h>

#include "Display/Screens/ui_text.h"
#include "Fonts.h"
#include "GPS/gps_config.h"
#include "GPS/Ublox/ublox_driver.h"
#include "Layout.h"

void draw_WAIT_SATS()
{
  drawCenteredText("ESP-GPS", Layout::ROW9(2), Fonts::Body12);
  drawCenteredText("Searching for Satellites", Layout::ROW9(4), Fonts::Body9);

  char buf[32];
  snprintf(buf, sizeof(buf), "Sat Fix %d of %d", ubxMessage.navPvt.numSV, MIN_numSV_FIRST_FIX);
  drawCenteredText(buf, Layout::ROW9(7), Fonts::Body9);
}
