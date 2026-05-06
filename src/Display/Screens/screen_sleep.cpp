#include "Display/Screens/screen_system.h"

#include "Config/config_types.h"
#include "Core/Rtc/rtc_session_stats.h"
#include "Display/E_paper.h"
#include "Display/Screens/ui_fixed_numbers.h"
#include "Fonts.h"
#include "Fonts/BitmapSurfbuddies.h"

void draw_SLEEP()
{
  constexpr int ROWS = 6;
  constexpr int ROW_START = 18;
  constexpr int ROW_STEP = 20;
  constexpr int COL_LABEL = 1;
  constexpr int COL_VALUE_RIGHT = 120;
  constexpr int DIV_X = 133;
  constexpr int INFO_X_L = 145;
  constexpr int INFO_Y = 75;
  constexpr int INFO_STEP = 20;

  const char* labels[ROWS] = { "02:", "10:", "1H:", "AL:", "NM:", "DI:" };
  const float values[ROWS] = {
    RTC_max_2s_knots,
    RTC_avg_10s_knots,
    RTC_1h_knots,
    RTC_alp_knots,
    RTC_mile_knots,
    RTC_distance
  };

  display.setFont(Fonts::Mono12);
  for (int i = 0; i < ROWS; ++i) {
    display.setCursor(COL_LABEL, ROW_START + i * ROW_STEP);
    display.print(labels[i]);
  }

  for (int i = 0; i < ROWS; ++i) {
    drawFixedNumber(COL_VALUE_RIGHT, ROW_START + i * ROW_STEP, values[i]);
  }

  display.drawFastVLine(DIV_X, 2, 118, GxEPD_BLACK);

  display.setFont(Fonts::Body9);
  display.setCursor(INFO_X_L, INFO_Y + 0 * INFO_STEP);
  display.print(config.Sleep_info1);

  display.setCursor(INFO_X_L, INFO_Y + 1 * INFO_STEP);
  display.print(config.Sleep_info2);

  display.setFont(Fonts::Mono9);
  display.setCursor(INFO_X_L, INFO_Y + 2 * INFO_STEP);
  display.print("Batt:XX%");

  display.drawBitmap(
    display.width() - 48 - 4,
    4,
    ESP_GPS_logo,
    48,
    48,
    GxEPD_WHITE,
    GxEPD_BLACK
  );
}
