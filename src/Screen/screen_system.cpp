#include <Arduino.h>

#include <WiFi.h>

#include "screen_system.h"
#include "screen_context.h"
#include "screen_ui.h"

#include "Layout.h"
#include "Fonts.h"
#include "esp_logo.h"

#include "storage_manager.h"
#include "wifi_manager.h"
#include "rtc_state.h"

// ============================================================================
// UI STATE
// ============================================================================
static int ui_offset = 0;

// ============================================================================
// LOCAL HELPERS (cpp-only)
// ============================================================================

static inline void beginScreen()
{
  display.setRotation(1);
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
}

static inline void drawTitle(const char* txt)
{
  display.setFont(Fonts::Body12);
  display.setCursor(ui_offset, Layout::ROW9(1));
  display.print(txt);
}

static inline void drawSpeedUnitsFooter()
{
  display.setRotation(0);
  display.setCursor(30, 249);
  display.setFont(Fonts::Small6);

  if ((int)(calibration_speed * 100000) == 194) {
    display.print("speed in knots");
  }
  else if ((int)(calibration_speed * 1000000) == 3600) {
    display.print("speed in km/h");
  }

  display.setRotation(1);
}

// ============================================================================
// OFF SCREEN (shutdown / save)
// ============================================================================

void Off_screen(int choice)
{
  const float session_time =
      (millis() - start_logging_millis) / 1000.0f;

  beginScreen();

  drawTitle("ESP-GPS saving");
  device_boot_log(4, 0);

  int cursor = Layout::ROW9(3) + Layout::STEP12;
  display.setCursor(ui_offset, cursor);

  // --- LOW BATTERY ---
  if (choice == 2) {
    display.println("Shutdown LOW Bat");

    display.setFont(Fonts::Body9);
    display.print("Bat = ");
    display.print(RTC_voltage_bat);
    display.println(" V");
  }
  // --- SAVE SESSION ---
  else if (Shut_down_Save_session) {

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
  }
  // --- NO SAVE ---
  else {
    display.println("Going back to sleep");
  }

  drawChrome(ui_offset, false);
  display.display(true);
}

// ============================================================================
// BOOT SCREEN
// ============================================================================

void draw_BOOT()
{
  display.firstPage();
  do {
    beginScreen();

    drawChrome(ui_offset, true);

    display.setFont(Fonts::Body9);
    display.setCursor(ui_offset, 14);

    // --------------------------------------------------
    // LOW BATTERY PATH
    // --------------------------------------------------
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

      break;   // draw once only
    }

  } while (display.nextPage());
}

// ============================================================================
// WIFI AP / CONFIG MODE
// ============================================================================

void draw_WIFI_SOFT_AP()
{
  display.firstPage();
  do {
    beginScreen();

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

  } while (display.nextPage());
}

// ============================================================================
// WIFI STATION / HOME MODE
// ============================================================================

void draw_WIFI_STATION()
{
  display.firstPage();
  do {
    beginScreen();

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
    display.print(staConnected
                  ? WiFi.localIP().toString().c_str()
                  : WiFi.softAPIP().toString().c_str());

    // --- USER HINT ---
    display.setFont(Fonts::Body9);
    display.setCursor(ui_offset, Layout::ROW9(8));
    display.print(staConnected ? "Access via home network"
                               : "Connect to device Wi-Fi");

  } while (display.nextPage());
}

// ============================================================================
// SLEEP SCREEN (RTC SUMMARY)
// ============================================================================

void Sleep_screen(int choice)
{
  ui_offset = constrain(ui_offset, 1, 9);

  display.init();
  beginScreen();
  drawChrome(ui_offset, true);

  // --------------------------------------------------
  // SIMPLE MODE
  // --------------------------------------------------
  if (choice == 0) {
    display.setFont(Fonts::Body18);

    display.setCursor(ui_offset, 24);
    display.printf("Dist: %.0f", RTC_distance);

    display.setCursor(ui_offset, 56);
    display.printf("AVG: %.2f", RTC_avg_10s);

    display.setCursor(ui_offset, 88);
    display.printf("2s: %.2f", RTC_max_2s);

    display.display();
    return;
  }

  // --------------------------------------------------
  // DETAILED MODE
  // --------------------------------------------------
  constexpr int rowStep = 15;

  const int row1 = 15;
  const int row2 = row1 + rowStep;
  const int row3 = row2 + rowStep;
  const int row4 = row3 + rowStep;
  const int row5 = row4 + rowStep;
  const int row6 = row5 + rowStep;

  const int col1 = ui_offset;
  const int col2 = ui_offset + 34;
  const int col3 = ui_offset + 90;
  const int col4 = ui_offset + 146;

  display.setFont(&SF_Distant_Galaxy9pt7b);
  display.setCursor(col1, 105);
  display.print(RTC_Sleep_txt);

  drawSpeedUnitsFooter();

  // --- LEFT COLUMN ---
  display.setFont(Fonts::Mono9);
  display.setCursor(col1, row1); display.print("AV:");
  display.setCursor(col1, row2); display.print("R1:");
  display.setCursor(col1, row3); display.print("R2:");
  display.setCursor(col1, row4); display.print("R3:");
  display.setCursor(col1, row5); display.print("R4:");
  display.setCursor(col1, row6); display.print("R5:");

  display.setFont(Fonts::Body9);
  display.setCursor(col2, row1); display.println(RTC_avg_10s, 2);
  display.setCursor(col2, row2); display.println(RTC_R1_10s, 2);
  display.setCursor(col2, row3); display.println(RTC_R2_10s, 2);
  display.setCursor(col2, row4); display.println(RTC_R3_10s, 2);
  display.setCursor(col2, row5); display.println(RTC_R4_10s, 2);
  display.setCursor(col2, row6); display.println(RTC_R5_10s, 2);

  // --- RIGHT COLUMN ---
  display.setFont(Fonts::Mono9);
  display.setCursor(col3, row1); display.print("2sec:");
  display.setCursor(col3, row2); display.print("Dist:");
  display.setCursor(col3, row3); display.print("Alph:");
  display.setCursor(col3, row4); display.print("1h:");
  display.setCursor(col3, row5); display.print("NM:");
  display.setCursor(col3, row6); display.print("500m:");

  display.setFont(Fonts::Body9);
  display.setCursor(col4, row1); display.println(RTC_max_2s, 2);
  display.setCursor(col4, row2); display.println(RTC_distance, 2);
  display.setCursor(col4, row3); display.println(RTC_alp, 2);
  display.setCursor(col4, row4); display.println(RTC_1h, 2);
  display.setCursor(col4, row5); display.println(RTC_mile, 2);
  display.setCursor(col4, row6); display.println(RTC_500m, 2);

  display.display();
}
