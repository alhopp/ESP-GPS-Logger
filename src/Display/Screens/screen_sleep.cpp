#include "Display/Screens/screen_system.h"

#include "Config/config_types.h"
#include "Core/Rtc/rtc_battery_state.h"
#include "Core/Rtc/rtc_session_stats.h"
#include "Display/Bitmaps/display_bitmaps.h"
#include "Display/E_paper.h"
#include "Core/Battery/battery_monitor.h"
#include "Display/Screens/ui_fixed_numbers.h"
#include "Fonts.h"

namespace {

constexpr int ROWS = 6;
constexpr int ROW_START = 18;
constexpr int ROW_STEP = 20;
constexpr int COL_LABEL = 1;
constexpr int COL_VALUE_RIGHT = 120;
constexpr int DIV_X = 133;
constexpr int INFO_X_L = 145;
constexpr int INFO_Y = 75;
constexpr int INFO_STEP = 20;

void drawStatsRows(const char* const labels[ROWS], const float values[ROWS])
{
  display.setFont(Fonts::Mono12);
  for (int i = 0; i < ROWS; ++i) {
    display.setCursor(COL_LABEL, ROW_START + i * ROW_STEP);
    display.print(labels[i]);
  }

  for (int i = 0; i < ROWS; ++i) {
    drawFixedNumber(COL_VALUE_RIGHT, ROW_START + i * ROW_STEP, values[i]);
  }
}

void drawInfoLine(int row, const char* text)
{
  display.setFont(Fonts::Body9);
  display.setCursor(INFO_X_L, INFO_Y + row * INFO_STEP);
  display.print(text);
}

void drawBatteryLine(int row)
{
  display.setFont(Fonts::Mono9);
  display.setCursor(INFO_X_L, INFO_Y + row * INFO_STEP);
  display.print("Batt:");
  display.print(static_cast<int>(battery_percent(RTC_voltage_bat)));
  display.print("%");
}

void drawLogo()
{
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

} // namespace

void draw_SLEEP()
{
  const char* labels[ROWS] = { "02:", "10:", "1H:", "AL:", "NM:", "DI:" };
  const bool hasStats = rtc_session_stats_valid();
  const float values[ROWS] = {
    hasStats ? RTC_max_2s_knots : 0.0f,
    hasStats ? RTC_avg_10s_knots : 0.0f,
    hasStats ? RTC_1h_knots : 0.0f,
    hasStats ? RTC_alp_knots : 0.0f,
    hasStats ? RTC_mile_knots : 0.0f,
    hasStats ? RTC_distance : 0.0f
  };

  drawStatsRows(labels, values);
  display.drawFastVLine(DIV_X, 2, 118, GxEPD_BLACK);

  drawInfoLine(0, config.Sleep_info1);
  drawInfoLine(1, config.Sleep_info2);
  drawBatteryLine(2);
  drawLogo();
}
