// -----------------------------------------------------------------------------
// screen_system.cpp
//
// Display-layer rendering only.
// - draw_*() functions paint pixels/text ONLY
// - No display.display(), no paging, no clearing
// - Display task owns refresh policy
// -----------------------------------------------------------------------------

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


// -----------------------------------------------------------------------------
// Local helpers
// -----------------------------------------------------------------------------
static void drawCenteredText(const char* text, int y, const GFXfont* font);
static void drawSystemLayout(const char* title, const char* subtitle,
                             const char* key1 = nullptr, const char* val1 = nullptr,
                             const char* key2 = nullptr, const char* val2 = nullptr);

// -----------------------------------------------------------------------------
// UI state
// -----------------------------------------------------------------------------
static int ui_offset = 0;

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
// MODE: IDLE  (magnet affordance)
// ============================================================================

// Magnet affordance geometry (UI-owned)
constexpr int MAG_X = 30, MAG_Y = 12; constexpr int MAG_R = 8;


// Partial window tightly covering affordance
constexpr int MAG_WIN_X = MAG_X - MAG_R - 2;
constexpr int MAG_WIN_Y = MAG_Y - MAG_R - 2;
constexpr int MAG_WIN_W = MAG_R * 2 + 4;
constexpr int MAG_WIN_H = MAG_R * 2 + 4;


// Public UI hook (called by input layer on state change)
void screen_request_magnet_affordance()
{
  screen_request_partial(MAG_WIN_X, MAG_WIN_Y, MAG_WIN_W, MAG_WIN_H);
}

void draw_IDLE()
{
  // Magnet state indicator:
  //   ◯ no magnet
  //   ● magnet present
  if (magnet_active) 
       display.fillCircle(MAG_X, MAG_Y, MAG_R, GxEPD_BLACK);
  else display.drawCircle(MAG_X, MAG_Y, MAG_R, GxEPD_BLACK);

  // Static UI
  //display.drawBitmap(200, 3, ESP_GPS_logo, 48, 48, GxEPD_WHITE, GxEPD_BLACK);
  drawCenteredText("ESP-GPS",                   Layout::ROW9(2), Fonts::Body12);
  drawCenteredText("Tap: Start",                Layout::ROW9(4), Fonts::Body9);
  drawCenteredText("Hold: Settings",            Layout::ROW9(5), Fonts::Body9);
  drawCenteredText("Use magnet to select mode", Layout::ROW9(7), Fonts::Body9);
}

void draw_WIFI_CONFIG()
{
  // Magnet affordance
  if (magnet_active)
       display.fillCircle(MAG_X, MAG_Y, MAG_R, GxEPD_BLACK);
  else display.drawCircle(MAG_X, MAG_Y, MAG_R, GxEPD_BLACK);

  drawCenteredText("CONFIG", Layout::ROW9(2), Fonts::Body12);

  switch (wifi_get_ui_state())
  {
    case WIFI_UI_TRYING:
      drawCenteredText("Connecting to hotspot", Layout::ROW9(4), Fonts::Body9);
      drawCenteredText("Open: gps.local",       Layout::ROW9(6), Fonts::Body9);
      break;

    case WIFI_UI_FAILED:
      drawCenteredText("Wi-Fi connection failed", Layout::ROW9(4), Fonts::Body9);
      drawCenteredText("Retrying…",               Layout::ROW9(6), Fonts::Body9);
      break;

    case WIFI_UI_AP:
      // If you are keeping this state internally, make it neutral
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





void draw_SLEEP()
{
  constexpr int ROWS      = 6;
  constexpr int ROW_START = 18;
  constexpr int ROW_STEP  = 20;

  constexpr int COL_LABEL = 10;
  constexpr int COL_VALUE_RIGHT = 135;   // right-aligned to screen edge

  const char* LABELS[ROWS] = {
    "02:", "10:", "1H:", "AL:", "NM:", "DI:"
  };

  const float VALUES[ROWS] = {
    39.87f,
    36.80f,
    23.45f,
    19.00f,
    31.46f,
   127.02f
  };

  // Labels
  display.setFont(Fonts::Mono12);
  for (int i = 0; i < ROWS; ++i) {
    const int y = ROW_START + i * ROW_STEP;
    display.setCursor(COL_LABEL, y);
    display.print(LABELS[i]);
  }

  // Numbers
  for (int i = 0; i < ROWS; ++i) {
    const int y = ROW_START + i * ROW_STEP;
    drawFixedNumber(COL_VALUE_RIGHT, y, VALUES[i]);
  }
}


// ============================================================================
// MODE: WAIT FOR SATS
// ============================================================================
void draw_WAIT_SATS()
{

  drawCenteredText("ESP-GPS",                    Layout::ROW9(2), Fonts::Body12);
  drawCenteredText("Searching for Satellites",    Layout::ROW9(4), Fonts::Body9);

  static char buf[32];
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
