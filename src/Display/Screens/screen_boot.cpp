#include "Display/Screens/screen_system.h"

#include "Core/Rtc/rtc_battery_state.h"
#include "Core/Battery/battery_monitor.h"
#include "Display/display_battery.h"
#include "Display/Bitmaps/display_bitmaps.h"
#include "Display/E_paper.h"
#include "Display/Screens/screen_system_common.h"
#include "Display/Screens/ui_text.h"
#include "Fonts.h"
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

  display.setFont(Fonts::Body9);
  display.setCursor(42, Layout::ROW9(7));
  display.print("Battery ");
  display.print(battery_display_voltage(RTC_voltage_bat), 1);
  display.print("V ");
  display.print(static_cast<int>(displayBatteryPercent(RTC_voltage_bat)));
  display.print("%");
}
