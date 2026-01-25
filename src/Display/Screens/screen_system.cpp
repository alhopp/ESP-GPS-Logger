// ============================================================================
// screen_system.cpp
//
// Display-layer rendering ONLY.
// - draw_*() functions paint pixels/text only
// - No display.display(), no paging, no clearing
// - Display task owns refresh policy
// ============================================================================

#include <Arduino.h>
#include <WiFi.h>

#include "Display/Screens/screen_system.h"
#include "Display/Screens/ui_fixed_numbers.h"
#include "Display/E_paper.h"

#include "Layout.h"
#include "Fonts.h"
#include "Fonts/BitmapSurfbuddies.h"

#include "Storage/storage_manager.h"
#include "web/wifi_manager.h"
#include "Core/rtc_state.h"

#include "core/magnet_input.h"
#include "core/system_mode.h"
#include "tasks/task_display.h"

#include "Ublox/ublox.h"

#include "MANAGERS/config_manager.h"

// ============================================================================
// Local helpers
// ============================================================================
static void drawCenteredText(const char* text, int y, const GFXfont* font);


// ============================================================================
// MODE: BOOT
// ============================================================================
void draw_BOOT()
{
  if (RTC_voltage_bat < RTC_minimum_voltage_bat) {
    drawCenteredText("SLEEP",           Layout::ROW9(2), Fonts::Body12);
    drawCenteredText("Battery too low", Layout::ROW9(5), Fonts::Body9);
    return;
  }

  display.drawBitmap(200, 3, ESP_GPS_logo, 48, 48, GxEPD_WHITE, GxEPD_BLACK);
  drawCenteredText("ESP-GPS",             Layout::ROW9(3), Fonts::Body12);
  drawCenteredText("Initialising system", Layout::ROW9(5), Fonts::Body9);
}


// ============================================================================
// MODE: IDLE / CONFIG (magnet affordance)
// ============================================================================

// Magnet affordance geometry (UI-owned)
constexpr int MAG_X = 30, MAG_Y = 12, MAG_R = 8;

// Partial window tightly covering affordance
constexpr int MAG_WIN_X = MAG_X - MAG_R - 2;
constexpr int MAG_WIN_Y = MAG_Y - MAG_R - 2;
constexpr int MAG_WIN_W = MAG_R * 2 + 4;
constexpr int MAG_WIN_H = MAG_R * 2 + 4;


// UI hook (called by input layer on state change)
void screen_request_magnet_affordance()
{
  screen_request_partial(MAG_WIN_X, MAG_WIN_Y, MAG_WIN_W, MAG_WIN_H);
}


// Draw magnet indicator (shared)
static inline void drawMagnet()
{
  if (magnet_active) display.fillCircle(MAG_X, MAG_Y, MAG_R, GxEPD_BLACK);
  else               display.drawCircle(MAG_X, MAG_Y, MAG_R, GxEPD_BLACK);
}


void draw_IDLE()
{
  drawMagnet();

  drawCenteredText("ESP-GPS",                   Layout::ROW9(2), Fonts::Body12);
  drawCenteredText("Tap: Start",                Layout::ROW9(4), Fonts::Body9);
  drawCenteredText("Hold: Settings",            Layout::ROW9(5), Fonts::Body9);
  drawCenteredText("Use magnet to select mode", Layout::ROW9(7), Fonts::Body9);
}


void draw_WIFI_CONFIG()
{
  drawMagnet();
  drawCenteredText("CONFIG", Layout::ROW9(2), Fonts::Body12);

  switch (wifi_get_ui_state()) {

    case WIFI_UI_TRYING:
      drawCenteredText("Connecting to hotspot", Layout::ROW9(4), Fonts::Body9);
      drawCenteredText("Open: gps.local",       Layout::ROW9(6), Fonts::Body9);
      break;

    case WIFI_UI_FAILED:
      drawCenteredText("Wi-Fi connection failed", Layout::ROW9(4), Fonts::Body9);
      drawCenteredText("Retrying…",               Layout::ROW9(6), Fonts::Body9);
      break;

    case WIFI_UI_AP:
      drawCenteredText("Wi-Fi setup required", Layout::ROW9(4), Fonts::Body9);
      drawCenteredText("Open: gps.local",      Layout::ROW9(6), Fonts::Body9);
      break;

    case WIFI_UI_CONNECTED:
      drawCenteredText("Wi-Fi connected", Layout::ROW9(4), Fonts::Body9);
      drawCenteredText("Open: gps.local", Layout::ROW9(6), Fonts::Body9);
      break;

    case WIFI_UI_OFF:
    default:
      drawCenteredText("Wi-Fi idle", Layout::ROW9(4), Fonts::Body9);
      drawCenteredText("Open: gps.local", Layout::ROW9(6), Fonts::Body9);
      break;
  }
}


// ============================================================================
// MODE: SLEEP (summary stats)
// ============================================================================
void draw_SLEEP()
{
  constexpr int ROWS = 6;
  constexpr int ROW_START = 18, ROW_STEP = 20;
  constexpr int COL_LABEL = 1;
  constexpr int COL_VALUE_RIGHT = 120;   // right-aligned anchor

  const char* LABELS[ROWS] = { "02:", "10:", "1H:", "AL:", "NM:", "DI:" };

  // Test values (replace with RTC values later)
  const float VALUES[ROWS] = {
    39.87f, 36.80f, 23.45f, 19.00f, 31.46f, 327.02f
  };

/* Real values (RTC snapshot)
const float VALUES[ROWS] = {
  RTC_max_2s,     // 02:
  RTC_avg_10s,    // 10:
  RTC_1h,         // 1H:
  RTC_alp,        // AL:
  RTC_mile,       // NM:
  RTC_distance    // DI:
};
*/

  // Labels (mono)
  display.setFont(Fonts::Mono12);
  for (int i = 0; i < ROWS; ++i) {
    display.setCursor(COL_LABEL, ROW_START + i * ROW_STEP);
    display.print(LABELS[i]);
  }

  // Values (fixed-width renderer)
  for (int i = 0; i < ROWS; ++i) {
    drawFixedNumber(COL_VALUE_RIGHT,
                    ROW_START + i * ROW_STEP,
                    VALUES[i]);
  }

  // -----------------------------------------------------------------------------
  // Vertical divider
  // -----------------------------------------------------------------------------
  constexpr int DIV_X = 133;
  display.drawFastVLine(DIV_X,2, 118, GxEPD_BLACK);
  // -----------------------------------------------------------------------------
  // Right-hand system info
  // -----------------------------------------------------------------------------
  constexpr int INFO_X_L = 145;   // label column
  constexpr int INFO_Y   =  75;
  constexpr int INFO_STEP = 20;

  // Values
  display.setFont(Fonts::Body9);
  display.setCursor(INFO_X_L, INFO_Y + 0 * INFO_STEP);
  display.print(config.Sleep_info1);

  display.setCursor(INFO_X_L, INFO_Y + 1 * INFO_STEP);
  display.print(config.Sleep_info2);         
 
  display.setFont(Fonts::Mono9);
  display.setCursor(INFO_X_L, INFO_Y + 2 * INFO_STEP);
  display.print("Batt:XX%");

  // -----------------------------------------------------------------------------
  // ESP logo (top-right)
  // -----------------------------------------------------------------------------
  display.drawBitmap(
    display.width() - 48 - 4,
    4,
    ESP_GPS_logo,
    48, 48,
    GxEPD_WHITE,
    GxEPD_BLACK
  );



}


// ============================================================================
// MODE: WAIT FOR SATS
// ============================================================================
void draw_WAIT_SATS()
{
  drawCenteredText("ESP-GPS",                 Layout::ROW9(2), Fonts::Body12);
  drawCenteredText("Searching for Satellites",Layout::ROW9(4), Fonts::Body9);

  char buf[32];
  snprintf(buf, sizeof(buf), "Sat Fix %d of 5", ubxMessage.navPvt.numSV);
  drawCenteredText(buf, Layout::ROW9(7), Fonts::Body9);
}


// ============================================================================
// Helpers
// ============================================================================
static void drawCenteredText(const char* text, int y, const GFXfont* font)
{
  int16_t x1, y1; uint16_t w, h;
  display.setFont(font);
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((display.width() - w) / 2, y);
  display.print(text);
}
