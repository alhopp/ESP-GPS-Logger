// ============================================================================
// display_chrome.cpp
//
// Shared display chrome:
// - Battery level
// - Satellite count
// - System / RTC time display
//
// IMPORTANT: Display chrome is read-only for time. GPS time is latched by the
// GPS task, not by display rendering.
// ============================================================================

#include "Display/display_chrome.h"

#include <Arduino.h>

#include "Display/E_paper.h"
#include "Fonts.h"
#include "Core/battery_config.h"
#include "Core/Globals.h"
#include "Core/Rtc/rtc_battery_state.h"
#include "Core/Rtc/rtc_time_state.h"
#include "GPS/Ublox/ublox_driver.h"

namespace {
constexpr int INFO_BAR_ROW_OFFSET = 2;

char timeNow[8];

float batteryPercent()
{
  const float percent = 100.0f * (1.0f - (VOLTAGE_100 - RTC_voltage_bat) / (VOLTAGE_100 - VOLTAGE_0));
  return constrain(percent, 0.0f, 100.0f);
}

bool updateTime()
{
  if (!getLocalTime(&tmstruct)) return false;
  snprintf(timeNow, sizeof(timeNow), "%02d:%02d", tmstruct.tm_hour, tmstruct.tm_min);
  return true;
}

void drawBattery(int uiOffset)
{
  constexpr int batW = 8;
  constexpr int batL = 15;
  const int posX = display.width() - batW - 6;
  const int posY = display.height() - batL;

  display.fillRect(uiOffset + posX, posY, batW / 2, batW / 4, GxEPD_BLACK);
  display.fillRect(uiOffset + posX - batW / 4, posY + batW / 4, batW, batL, GxEPD_BLACK);

  display.setFont(Fonts::Body9);
  display.setCursor(uiOffset + 146, display.height() - INFO_BAR_ROW_OFFSET);
  display.print(RTC_voltage_bat + 0.04, 1);
  display.print("V ");
  display.print(static_cast<int>(batteryPercent()));
  display.print("%");
}

void drawSatellites(int uiOffset)
{
  const int satNum = ubxMessage.navPvt.numSV;
  display.setFont(Fonts::Body9);
  display.setCursor(120 + uiOffset - (satNum < 10 ? 9 : 18), display.height() - INFO_BAR_ROW_OFFSET);
  display.print(satNum);
}

void drawGpsTime(int uiOffset)
{
  display.setFont(Fonts::Body9);
  display.setCursor(uiOffset, display.height() - INFO_BAR_ROW_OFFSET);

  if (!updateTime()) {
    display.print("--:--");
    return;
  }

  display.print(timeNow);
}

void drawRtcDateTime(int uiOffset)
{
  display.setFont(Fonts::Body9);
  display.setCursor(uiOffset, display.height() - INFO_BAR_ROW_OFFSET);
  display.printf("%02d:%02d %02d-%02d-%02d", RTC_hour, RTC_min, RTC_day, RTC_month, RTC_year);
}
} // namespace

void drawChrome(int offset, bool rtcMode)
{
  drawBattery(offset);

  if (rtcMode) {
    drawRtcDateTime(offset);
    return;
  }

  drawSatellites(offset);
  drawGpsTime(offset);
}
