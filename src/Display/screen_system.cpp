// -----------------------------------------------------------------------------
// screen_system.cpp
//
// "Guru" edition: clean, predictable, display-task-friendly screens.
//
// RULES (enforced here):
//  - draw_*() functions ONLY draw pixels/text. No firstPage/nextPage.
//  - No display.display(), no fillScreen()
//  - The display task owns paging + clearing + refresh policy.
//  - Keep UI state minimal + explicit (no hidden allocations).
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <WiFi.h>

#include "Display/screen_system.h"

#include "Layout.h"
#include "Fonts.h"
#include "logos.h"

#include "Storage/storage_manager.h"
#include "web/wifi_manager.h"
#include "rtc_state.h"

#include "Display/E_paper.h"
#include "Fonts/BitmapSurfbuddies.h"

// Forward declarations 
static void drawSystemLayout(const char* title, const char* subtitle, const char* key1,
  const char* val1, const char* key2, const char* val2);

static void drawCenteredText(const char* text, int y, const GFXfont* font);

static void drawCenteredBitmap(const uint8_t* bmp, int w, int h, int y);

// ============================================================================
// UI STATE (avoid globals where possible; keep deterministic)
// ============================================================================
static int ui_offset = 0;


// ============================================================================
// MODE: BOOT  (early boot screen / low-battery warning)
// ============================================================================
void draw_BOOT()
{
  // --------------------------------------------------
  // LOW BATTERY PATH
  // --------------------------------------------------
  if (RTC_voltage_bat < RTC_minimum_voltage_bat) {

    static char voltBuf[16];
    snprintf(voltBuf, sizeof(voltBuf), "%.2f V", RTC_voltage_bat);

    drawCenteredText("ESP-GPS SLEEPING", Layout::ROW9(3), Fonts::Body12);
    drawCenteredText("Battery too low",  Layout::ROW9(5), Fonts::Body9 );
    
    return;
  }

  // Logo
  display.drawBitmap(198, 6, ESP_GPS_logo, 48, 48, GxEPD_WHITE, GxEPD_BLACK )  ;
  drawCenteredText("ESP-GPS",             Layout::ROW9(3), Fonts::Body12);
  drawCenteredText("Initialising system", Layout::ROW9(5), Fonts::Body9 );

}

// ============================================================================
// MODE: IDLE  (post-wake / waiting for user intent)
// ============================================================================
void draw_IDLE()
{
  // Logo
  display.drawBitmap(198, 6, ESP_GPS_logo, 48, 48, GxEPD_WHITE, GxEPD_BLACK )  ;

  // Headi
  drawCenteredText("ESP-GPS",             Layout::ROW9(3), Fonts::Body12);
  
  // ---- Instructions ----
  display.setFont(Fonts::Body9);
  drawCenteredText("Hold 2s \xE2\x86\x92 START",  Layout::ROW9(4), Fonts::Body9);
  drawCenteredText("Hold 4s \xE2\x86\x92 CONFIG", Layout::ROW9(5), Fonts::Body9);


}



void draw_WAIT_SATS()
{
  static char satsBuf[32];
  snprintf(satsBuf, sizeof(satsBuf), "Sat Fix %d of 5", ubxMessage.navPvt.numSV);
  drawCenteredText(satsBuf, Layout::ROW9(7), Fonts::Body9 );

}





void draw_WIFI_SOFT_AP()
{
  const bool staConnected = wifi_sta_connected();

  drawSystemLayout(
    "CONFIG MODE",
    staConnected
      ? "Internet connected"
      : "Connect via phone",

    "WiFi",
    staConnected
      ? wifi_sta_ssid().c_str()
      : wifi_ap_name(),

    "IP",
    staConnected
      ? wifi_sta_ip().c_str()
      : WiFi.softAPIP().toString().c_str()
  );
}

// ============================================================================
// MODE: SLEEP  (shutdown / save progress screen)
// ============================================================================
void draw_SLEEP()

{
  constexpr int ROW_COUNT = 6;
  constexpr int ROW_STEP  = 15;
  constexpr int ROW_START = 15;

  // Column positions (relative to ui_offset)
  const int COL_LBL_L = ui_offset;
  const int COL_VAL_L = ui_offset + 34;
  const int COL_LBL_R = ui_offset + 90;
  const int COL_VAL_R = ui_offset + 146;

  // Row Y positions
  int rowY[ROW_COUNT];
  for (int i = 0; i < ROW_COUNT; ++i) {
    rowY[i] = ROW_START + i * ROW_STEP;
  }

  // -------------------------------------------------------------------------
  // Footer / sleep reason
  // -------------------------------------------------------------------------
  display.setFont(&SF_Distant_Galaxy9pt7b);
  display.setCursor(COL_LBL_L, 105);


  // -------------------------------------------------------------------------
  // Table data (static, deterministic)
  // -------------------------------------------------------------------------
  const char* LEFT_LABELS[ROW_COUNT] = {
    "AV:", "R1:", "R2:", "R3:", "R4:", "R5:"
  };

  const float LEFT_VALUES[ROW_COUNT] = {
    RTC_avg_10s,
    RTC_R1_10s,
    RTC_R2_10s,
    RTC_R3_10s,
    RTC_R4_10s,
    RTC_R5_10s
  };

  const char* RIGHT_LABELS[ROW_COUNT] = {
    "2sec:", "Dist:", "Alph:", "1h:", "NM:", "500m:"
  };

  const float RIGHT_VALUES[ROW_COUNT] = {
    RTC_max_2s,
    RTC_distance,
    RTC_alp,
    RTC_1h,
    RTC_mile,
    RTC_500m
  };

  // -------------------------------------------------------------------------
  // Draw labels
  // -------------------------------------------------------------------------
  display.setFont(Fonts::Mono9);
  for (int i = 0; i < ROW_COUNT; ++i) {
    display.setCursor(COL_LBL_L, rowY[i]);
    display.print(LEFT_LABELS[i]);

    display.setCursor(COL_LBL_R, rowY[i]);
    display.print(RIGHT_LABELS[i]);
  }

  // -------------------------------------------------------------------------
  // Draw values
  // -------------------------------------------------------------------------
  display.setFont(Fonts::Body9);
  for (int i = 0; i < ROW_COUNT; ++i) {
    display.setCursor(COL_VAL_L, rowY[i]);
    display.print(LEFT_VALUES[i], 2);

    display.setCursor(COL_VAL_R, rowY[i]);
    display.print(RIGHT_VALUES[i], 2);
  }
}


static void drawCenteredText(const char* text, int y, const GFXfont* font)
{
    int16_t x1, y1;
    uint16_t w, h;

    display.setFont(font);
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

    display.setCursor((display.width() - w) / 2, y);
    display.print(text);
}


// ============================================================================
// Shared system screen layout
// - Title
// - Subtitle
// - Two optional key/value rows
// ============================================================================

static void drawSystemLayout(
  const char* title,
  const char* subtitle,
  const char* key1 = nullptr,
  const char* val1 = nullptr,
  const char* key2 = nullptr,
  const char* val2 = nullptr
)
{

  // --- TITLE ---
  display.setFont(Fonts::Body12);
  display.setCursor(ui_offset, Layout::ROW9(2));
  display.print(title);

  // --- SUBTITLE ---
  if (subtitle) {
    display.setFont(Fonts::Body9);
    display.setCursor(ui_offset, Layout::ROW9(3));
    display.print(subtitle);
  }

  // --- ROW 1 ---
  if (key1 && val1) {
    display.setFont(Fonts::Mono12);
    display.setCursor(ui_offset, Layout::ROW9(5));
    display.printf("%4s:", key1);

    display.setFont(Fonts::Body9);
    display.print(" ");
    display.print(val1);
  }

  // --- ROW 2 ---
  if (key2 && val2) {
    display.setFont(Fonts::Mono12);
    display.setCursor(ui_offset, Layout::ROW9(6));
    display.printf("%4s:", key2);

    display.setFont(Fonts::Body9);
    display.print(" ");
    display.print(val2);
  }
}
