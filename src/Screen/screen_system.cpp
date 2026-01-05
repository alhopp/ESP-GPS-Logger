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

#include "screen_system.h"
#include "screen_context.h"

#include "Layout.h"
#include "Fonts.h"
#include "esp_logo.h"

#include "storage_manager.h"
#include "wifi_manager.h"
#include "rtc_state.h"

#include "E_paper.h"

// ============================================================================
// UI STATE (avoid globals where possible; keep deterministic)
// ============================================================================
static int ui_offset = 0;

// Small helper: keep ui_offset within a sane range (your original intent)
static inline int clampUiOffset(int v)
{
  return constrain(v, 1, 9);
}



// ============================================================================
// MODE: SLEEP  (shutdown / save progress screen)
// ============================================================================
void draw_SLEEP()
{
  const float session_time = (millis() - start_logging_millis) / 1000.0f;

  // "Chrome" is optional and per-screen. If you want less clutter during sleep,
  // you can remove this, but it's safe to keep.
  drawChrome(ui_offset, false);

  drawTopLeftTitle("ESP-GPS saving");
  device_boot_log(4, 0);

  int cursor = Layout::ROW9(3) + Layout::STEP12;
  display.setCursor(ui_offset, cursor);

  // --- LOW BATTERY ---
  if (RTC_voltage_bat < RTC_minimum_voltage_bat) {
    display.println("Shutdown LOW Bat");

    display.setFont(Fonts::Body9);
    display.print("Bat = ");
    display.print(RTC_voltage_bat);
    display.println(" V");
    return;
  }

  // --- SAVE SESSION ---
  if (Shut_down_Save_session) {
    display.println("Saving session");
    display.setFont(Fonts::Body9);

    display.setCursor(ui_offset, cursor += Layout::STEP9);
    display.print("Time: ");
    display.print(session_time, 0);
    display.print(" s");

    display.setCursor(ui_offset, cursor += Layout::STEP9);
    display.print("AVG: ");
    display.print(RTC_avg_10s, 2);

    display.setCursor(ui_offset + 120, cursor);
    display.print("Dist: ");
    display.print(Ublox.total_distance / 1000, 0);
    return;
  }

  // --- NO SAVE ---
  display.println("Going back to sleep");
}

// ============================================================================
// MODE: BOOT  (early boot screen / low-battery warning)
// ============================================================================
void draw_BOOT()
{
  drawChrome(ui_offset, true);

  display.setFont(Fonts::Body9);
  display.setCursor(ui_offset, 14);

  // --- LOW BATTERY PATH ---
  if (RTC_voltage_bat < RTC_minimum_voltage_bat) {
    display.println("ESP-GPS sleeping");
    display.print("Go back to sleep...");

    display.setFont(Fonts::Body12);
    display.setCursor(ui_offset, 60);
    display.printf("Voltage too low: %.2f", RTC_voltage_bat);

    display.setCursor(ui_offset, 80);
    display.println("Please charge lipo!");

    display.setCursor(ui_offset, 100);
    display.print(RTC_Sleep_txt);
    return;
  }

  // --- NORMAL BOOT PATH ---
  display.println("Booting...");
}


void beginScreen()
{
  display.setRotation(1);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(0, 0);
}


// ============================================================================
// MODE: WAIT_SATS  (GPS acquiring / no-fix yet)
// ============================================================================
void draw_WAIT_SATS()
{
  static int ui_offset = 0;

  //beginScreen();
  //display.setRotation(1);
  //display.setTextColor(GxEPD_BLACK);
  // Use ui_offset consistently (avoid magic 0 unless intentional)
  //drawChrome(ui_offset, true);

  display.setFont(Fonts::Body12);
  display.setCursor(ui_offset, 40);
  display.print("Waiting for GPS");

  display.setFont(Fonts::Body9);
  display.setCursor(ui_offset, 60);
  display.print("Acquiring satellites");

  display.setCursor(ui_offset, 80);
  display.print("Please wait...");

  display.print("Sats: ");
  display.print(ubxMessage.navPvt.numSV);


  Serial.print("wait sats");

}

// ============================================================================
// MODE: WIFI_SOFT_AP  (config mode / captive portal)
// ============================================================================
void draw_WIFI_SOFT_AP()
{
  drawChrome(ui_offset, true);

  // --- TITLE ---
  display.setFont(Fonts::Body12);
  display.setCursor(ui_offset, Layout::ROW9(2));
  display.print("CONFIG MODE");

  // --- INSTRUCTION ---
  display.setFont(Fonts::Body9);
  display.setCursor(ui_offset, Layout::ROW9(3));
  display.print("Connect via phone");

  // --- NETWORK INFO ---
  display.setFont(Fonts::Mono12);
  display.setCursor(ui_offset, Layout::ROW9(5));
  display.printf("%4s:", "WiFi");

  display.setFont(Fonts::Body9);
  display.print(" ");
  display.print(wifi_ap_name());

  display.setFont(Fonts::Mono12);
  display.setCursor(ui_offset, Layout::ROW9(6));
  display.printf("%4s:", "IP");

  display.setFont(Fonts::Body9);
  display.print(" ");
  display.print(WiFi.softAPIP().toString().c_str());
}

// ============================================================================
// MODE: WIFI_STATION  (connected to AP / home mode)
// ============================================================================
void draw_WIFI_STATION()
{
  drawChrome(ui_offset, true);

  display.setFont(Fonts::Body12);
  display.setCursor(ui_offset, Layout::ROW9(2));
  display.print("BEACH MODE");

  display.setFont(Fonts::Body9);
  display.setCursor(ui_offset, Layout::ROW9(3));

  const bool staConnected = (WiFi.status() == WL_CONNECTED);
  display.print(staConnected ? "Internet connected"
                             : "Device Wi-Fi only");

  // --- WIFI ---
  display.setFont(Fonts::Mono12);
  display.setCursor(ui_offset, Layout::ROW9(5));
  display.printf("%4s:", "WiFi");

  display.setFont(Fonts::Body9);
  display.print(" ");
  display.print(staConnected ? WiFi.SSID().c_str()
                             : "ESP32 GPS (AP)");

  // --- IP ---
  display.setFont(Fonts::Mono12);
  display.setCursor(ui_offset, Layout::ROW9(6));
  display.printf("%4s:", "IP");

  display.setFont(Fonts::Body9);
  display.print(" ");
  display.print(staConnected ? WiFi.localIP().toString().c_str()
                             : WiFi.softAPIP().toString().c_str());

  // --- USER HINT ---
  display.setFont(Fonts::Body9);
  display.setCursor(ui_offset, Layout::ROW9(8));
  display.print(staConnected ? "Access via home network"
                             : "Connect to device Wi-Fi");
}

// ============================================================================
// LEGACY BRIDGE: Sleep_screen(choice)
//
// This is NOT a mode-level screen. It is a view/helper used by old call sites.
// In the new architecture, prefer MODE_SLEEP + draw_SLEEP().
// Keep it deterministic, draw-only, and DO NOT call display.display().
// ============================================================================
void Sleep_screen(int choice)
{
  ui_offset = clampUiOffset(ui_offset);

  drawChrome(ui_offset, true);

  // ---------------------------------------------------------------------------
  // SIMPLE MODE
  // ---------------------------------------------------------------------------
  if (choice == 0) {
    display.setFont(Fonts::Body18);

    display.setCursor(ui_offset, 24);
    display.printf("Dist: %.0f", RTC_distance);

    display.setCursor(ui_offset, 56);
    display.printf("AVG: %.2f", RTC_avg_10s);

    display.setCursor(ui_offset, 88);
    display.printf("2s: %.2f", RTC_max_2s);

    return;
  }

  // ---------------------------------------------------------------------------
  // DETAILED MODE (RTC summary table)
  // ---------------------------------------------------------------------------
  constexpr int rowStep = 15;
  constexpr int row0    = 15;

  const int rows[6] = {
    row0 + rowStep * 0,
    row0 + rowStep * 1,
    row0 + rowStep * 2,
    row0 + rowStep * 3,
    row0 + rowStep * 4,
    row0 + rowStep * 5
  };

  const int col1 = ui_offset;
  const int col2 = ui_offset + 34;
  const int col3 = ui_offset + 90;
  const int col4 = ui_offset + 146;

  // Footer message
  display.setFont(&SF_Distant_Galaxy9pt7b);
  display.setCursor(col1, 105);
  display.print(RTC_Sleep_txt);

  // Labels + values (static arrays: no heap, deterministic)
  const char* leftLbl[6]  = { "AV:", "R1:", "R2:", "R3:", "R4:", "R5:" };
  const float leftVal[6]  = { RTC_avg_10s, RTC_R1_10s, RTC_R2_10s,
                              RTC_R3_10s, RTC_R4_10s, RTC_R5_10s };

  const char* rightLbl[6] = { "2sec:", "Dist:", "Alph:", "1h:", "NM:", "500m:" };
  const float rightVal[6] = { RTC_max_2s, RTC_distance, RTC_alp,
                              RTC_1h, RTC_mile, RTC_500m };

  // Column labels
  display.setFont(Fonts::Mono9);
  for (int i = 0; i < 6; i++) {
    display.setCursor(col1, rows[i]); display.print(leftLbl[i]);
    display.setCursor(col3, rows[i]); display.print(rightLbl[i]);
  }

  // Column values
  display.setFont(Fonts::Body9);
  for (int i = 0; i < 6; i++) {
    display.setCursor(col2, rows[i]); display.println(leftVal[i], 2);
    display.setCursor(col4, rows[i]); display.println(rightVal[i], 2);
  }
}
