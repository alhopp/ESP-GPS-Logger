// ============================================================================
// E_paper.cpp
//
// Responsibilities:
// - Own the physical e-paper display object
// - Provide low-level UI chrome (battery, sats, time, info bar)
// - Provide boot / diagnostic drawing helpers
// - Provide legacy screen update shim (Update_screen)
//
// Non-responsibilities (IMPORTANT):
// - Does NOT decide which screen to draw
// - Does NOT dispatch draw_*() functions
// - Does NOT track SystemMode
// ============================================================================

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Display hardware + fonts (CORE — KEEP)
// -----------------------------------------------------------------------------
#include "E_paper.h"
#include "Fonts.h"

// -----------------------------------------------------------------------------
// Runtime data dependencies (READ-ONLY from UI)
// -----------------------------------------------------------------------------
#include "Ublox.h"
#include "GPS_data.h"
#include "Definitions.h"
#include "Globals.h"

// -----------------------------------------------------------------------------
// Storage / filesystem (boot info + info bar only)
// -----------------------------------------------------------------------------
#include "SD_card.h"
#include <LittleFS.h>
#include "storage_manager.h"

// -----------------------------------------------------------------------------
// UI primitives & layout (KEEP)
// -----------------------------------------------------------------------------
#include "Layout.h"

// -----------------------------------------------------------------------------
// System / config (boot + status text only)
// -----------------------------------------------------------------------------
#include "screen_system.h"
#include "config_manager.h"
#include "esp_logo.h"

// -----------------------------------------------------------------------------
// Display task signalling (CRITICAL — KEEP)
// -----------------------------------------------------------------------------
#include "task_display.h"

// ============================================================================
// Display instance (OWNED HERE)
// ============================================================================

GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(
  GxEPD2_213_B74(ELINK_SS, ELINK_DC, ELINK_RESET, ELINK_BUSY)
);

int16_t displayWidth  = 0;
int16_t displayHeight = 0;

// ============================================================================
// Local UI state (candidate for later pruning)
// ============================================================================

static int ui_offset = 0;

// Bottom info bar
#define INFO_BAR_HEIGHT 15
#define INFO_BAR_ROW (display.height() - 2)

// ============================================================================
// Chrome helpers — forward declarations
// (UI primitives — KEEP)
// ============================================================================

void Bat_level_Simon(int ui_offset);
void Sats_level(int ui_offset);
void M8_M10(int ui_offset);
int  Time(int ui_offset);
int  DateTimeRtc(int ui_offset);

// -----------------------------------------------------------------------------
// Unified chrome renderer
// -----------------------------------------------------------------------------
void drawChrome(int offset, bool rtcMode)
{
  Bat_level_Simon(offset);

  if (rtcMode) {
    DateTimeRtc(offset);
  } else {
    Sats_level(offset);
    if (ubxMessage.navPvt.numSV > 4) {
      M8_M10(offset);
    }
    Time(offset);
  }
}

// ============================================================================
// Device identification (BOOT ONLY — KEEP)
// ============================================================================

#if defined(EPD_213_B74)
  const char E_paper_version[] = "E-paper 213B74";
#elif defined(EPD_213_B73)
  const char E_paper_version[] = "E-paper 213B73";
#elif defined(EPD_266_BN)
  const char E_paper_version[] = "E-paper 266BN";
#else
  const char E_paper_version[] = "E-paper unknown";
#endif

// ============================================================================
// Boot / diagnostics drawing
// (Used only during boot screens — KEEP)
// ============================================================================

int device_boot_log(int rows, int ws)
{
  int r = 2;

  auto pause = [&]() {
    if (ws) delay(ws);
  };

  // Device + firmware header
  display.setCursor(ui_offset, Layout::ROW9(2));
  pause();
  display.print(E_paper_version);
  display.print(SW_version);

  // Storage info
  const bool show_storage =
      (rows == 2 || rows == 23 || rows == 24 || rows == 234);

  if (show_storage) {
    display.setCursor(ui_offset, Layout::ROW9(3));
    pause();
    sdCardInfo();
  }

  // Cursor advance logic
  const bool advance_row =
      (rows == 3 || rows == 23 || rows == 34 || rows == 234);

  if (advance_row) {
    r = (rows == 234) ? 4
        : (rows == 23 || rows == 34) ? 3
        : 2;

    display.setCursor(
      ui_offset,
      (rows == 234 || rows == 23 || rows == 34)
        ? Layout::ROW9(4)
        : Layout::ROW9(3)
    );
  }

  // GPS info
  const bool show_gps =
      (rows == 4 || rows == 24 || rows == 34 || rows == 234) &&
      ubxMessage.monVER.hwVersion[0];

  if (show_gps) {
    display.setCursor(
      ui_offset,
      (rows == 234) ? Layout::ROW9(5)
      : (rows == 24 || rows == 34) ? Layout::ROW9(4)
      : Layout::ROW9(3)
    );

    display.printf("Gps %s at %dHz",
                    gpsChip(1),
                    config.sample_rate);
  }

  return r;
}

#define device_boot_log(rows) device_boot_log(rows, 0)

// ============================================================================
// Time helpers (INFO BAR — KEEP)
// ============================================================================

char time_now[8];
char time_now_sec[12];

int update_time()
{
  if (!NTP_time_set && !Gps_time_set) {
    if (Set_GPS_Time(config.timezone)) {
      Gps_time_set = 1;
    }
  }

  if ((!Gps_time_set && !NTP_time_set) || !getLocalTime(&tmstruct)) {
    return 1;
  }

  sprintf(time_now, "%02d:%02d", tmstruct.tm_hour, tmstruct.tm_min);
  sprintf(time_now_sec, "%02d:%02d:%02d",
          tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);

  return 0;
}

// ============================================================================
// Battery / satellite / time chrome (KEEP — UI primitives)
// ============================================================================

void Bat_level_Simon(int ui_offset)
{
  float bat_perc = 100 * (1 - (VOLTAGE_100 - RTC_voltage_bat) /
                                (VOLTAGE_100 - VOLTAGE_0));
  bat_perc = constrain(bat_perc, 0, 100);

  int batW = 8;
  int batL = 15;
  int posX = display.width() - batW - 6;
  int posY = display.height() - batL;

  display.fillRect(ui_offset + posX, posY, batW / 2, batW / 4, GxEPD_BLACK);
  display.fillRect(ui_offset + posX - batW / 4,
                   posY + batW / 4,
                   batW, batL, GxEPD_BLACK);

  display.setFont(Fonts::Body9);
  display.setCursor(ui_offset + 146, INFO_BAR_ROW);
  display.print(RTC_voltage_bat + 0.04, 1);
  display.print("V ");
  display.print((int)bat_perc);
  display.print("%");
}

void Sats_level(int ui_offset)
{
  if (!ubxMessage.monVER.swVersion[0]) return;

  int satnum = ubxMessage.navPvt.numSV;
  display.setFont(Fonts::Body9);
  display.setCursor(120 + ui_offset - (satnum < 10 ? 9 : 18), INFO_BAR_ROW);
  display.print(satnum);
}

void M8_M10(int ui_offset)
{
  display.setFont(Fonts::Body9);
  display.setCursor(ui_offset + 60, INFO_BAR_ROW);
  display.print(gpsChip(0));
}

int Time(int ui_offset)
{
  if (!update_time()) {
    display.setFont(Fonts::Body9);
    display.setCursor(ui_offset, INFO_BAR_ROW);
    display.print(time_now);
  }
  return 0;
}

int DateTimeRtc(int ui_offset)
{
  display.setFont(Fonts::Body9);
  display.setCursor(ui_offset, INFO_BAR_ROW);
  display.printf("%02d:%02d %02d-%02d-%02d",
                 RTC_hour, RTC_min, RTC_day, RTC_month, RTC_year);
  return 0;
}

// ============================================================================
// Storage info (BOOT + INFO — KEEP)
// ============================================================================

void sdCardInfo()
{
  if (sdOK) {
    display.printf("SD    : %llu MB\n",
                   storageFreeKBytes() / 1024);
  }
  else if (LITTLEFS_OK) {
    display.printf("Local : %llu KB\n",
                   storageFreeKBytes());
  }
}

// ============================================================================
// Legacy screen update API (DO NOT REMOVE)
// ============================================================================
//
// Screen selection is now driven by SystemMode and rendered by task_display.
// This function exists only to trigger a redraw for legacy callers.
//
void Update_screen(int /*screen*/)
{
  screen_request_redraw();
}
