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

#include "Display/screen_system.h"
#include "Display/E_paper.h"

#include "Layout.h"
#include "Fonts.h"
#include "logos.h"
#include "Fonts/BitmapSurfbuddies.h"

#include "Storage/storage_manager.h"
#include "web/wifi_manager.h"
#include "rtc_state.h"

#include "magnet_input.h"
#include "system_mode.h"
#include "task_display.h"

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

// ============================================================================
// MODE: WIFI CONFIG (SoftAP)
// ============================================================================
void draw_WIFI_SOFT_AP()
{
  // --- Magnet affordance ---
  if (magnet_active)
       display.fillCircle(MAG_X, MAG_Y, MAG_R, GxEPD_BLACK);
  else display.drawCircle(MAG_X, MAG_Y, MAG_R, GxEPD_BLACK);

  // title
  drawCenteredText("CONFIG", Layout::ROW9(2), Fonts::Body12);

  // ESP32 SoftAP details
  static char wifiLine[48]; static char ipLine[32];

  //snprintf(wifiLine, sizeof(wifiLine),        "WiFi: %s", wifi_ap_name());
  //snprintf(ipLine,   sizeof(ipLine),          "IP: %s", WiFi.softAPIP().toString().c_str());

  drawCenteredText(wifiLine,                  Layout::ROW9(4), Fonts::Body9);
  drawCenteredText(ipLine,                    Layout::ROW9(5), Fonts::Body9);
  drawCenteredText("Use phone to connect",    Layout::ROW9(7), Fonts::Body9);
}


// ============================================================================
// MODE: SLEEP (summary screen)
// ============================================================================
void draw_SLEEP()
{
  // --- Magnet affordance ---
  if (magnet_active)
       display.fillCircle(MAG_X, MAG_Y, MAG_R, GxEPD_BLACK);
  else display.drawCircle(MAG_X, MAG_Y, MAG_R, GxEPD_BLACK);
  
  constexpr int ROWS = 6, STEP = 15, START = 15;
  const int L0 = ui_offset+20 , V0 = ui_offset + 34;
  const int L1 = ui_offset + 90, V1 = ui_offset + 146;

  const char* LBL_L[ROWS] = {"AV:","R1:","R2:","R3:","R4:","R5:"};
  const char* LBL_R[ROWS] = {"2sec:","Dist:","Alph:","1h:","NM:","500m:"};

  const float VAL_L[ROWS] = {
    RTC_avg_10s, RTC_R1_10s, RTC_R2_10s,
    RTC_R3_10s,  RTC_R4_10s, RTC_R5_10s
  };

  const float VAL_R[ROWS] = {
    RTC_max_2s, RTC_distance, RTC_alp,
    RTC_1h, RTC_mile, RTC_500m
  };

  //display.setFont(Fonts::Mono9);
  //for (int i = 0; i < ROWS; ++i) {
  //  const int y = START + i * STEP;
  //  display.setCursor(L0, y); display.print(LBL_L[i]);
  //  display.setCursor(L1, y); display.print(LBL_R[i]);
 // }

  //display.setFont(Fonts::Body9);
  //for (int i = 0; i < ROWS; ++i) {
  //  const int y = START + i * STEP;
  //  display.setCursor(V0, y); display.print(VAL_L[i], 2);
  //  display.setCursor(V1, y); display.print(VAL_R[i], 2);
 // }

  drawCenteredText("Short magnet to start",    Layout::ROW9(7), Fonts::Body9);

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
