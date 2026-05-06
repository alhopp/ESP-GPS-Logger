#include "Display/Screens/screen_system.h"

#include "Core/Rtc/rtc_battery_state.h"
#include "Display/E_paper.h"
#include "Display/Screens/screen_system_common.h"
#include "Display/Screens/ui_text.h"
#include "Fonts.h"
#include "Fonts/BitmapSurfbuddies.h"
#include "Layout.h"

void draw_BOOT()
{
  if (RTC_voltage_bat < RTC_minimum_voltage_bat) {
    drawCenteredText("SLEEP", Layout::ROW9(2), Fonts::Body12);
    drawCenteredText("Battery too low", Layout::ROW9(5), Fonts::Body9);
    return;
  }

  display.drawBitmap(200, 3, ESP_GPS_logo, 48, 48, GxEPD_WHITE, GxEPD_BLACK);
  drawCenteredText("ESP-GPS", Layout::ROW9(3), Fonts::Body12);
  drawCenteredText("Initialising system", Layout::ROW9(5), Fonts::Body9);
}
