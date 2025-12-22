#include <Arduino.h>

#include "screen_system.h"
#include "screen_context.h"
#include "Layout.h"
#include "Fonts.h"
#include "screen_ui.h"
#include "storage_manager.h"
#include "esp_logo.h"

/* =========================================================
 * Local helpers (cpp-only)
 * ========================================================= */

static inline void beginScreen()
{
  display.setRotation(1);
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
}

static inline void drawTitle(const char* txt)
{
  display.setFont(Fonts::Body12);
  display.setCursor(offset, Layout::ROW9(1));
  display.print(txt);
}

static inline void drawSpeedUnitsFooter()
{
  display.setRotation(0);
  display.setCursor(30, 249);
  display.setFont(Fonts::Small6);

  if ((int)(calibration_speed * 100000) == 194)
    display.print("speed in knots");
  else if ((int)(calibration_speed * 1000000) == 3600)
    display.print("speed in km/h");

  display.setRotation(1);
}

/* =========================================================
 * Off screen (shutdown / save)
 * ========================================================= */

void Off_screen(int choice)
{
  const float session_time =
      (millis() - start_logging_millis) / 1000.0f;

  beginScreen();


  drawTitle("ESP-GPS saving");
  device_boot_log(4, 0);

  int cursor = Layout::ROW9(3) + Layout::STEP12;
  display.setCursor(offset, cursor);

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

    display.setCursor(offset, cursor += Layout::STEP9);
    display.print("Time: ");
    display.print(session_time, 0);
    display.print(" s");

    display.setCursor(offset, cursor += Layout::STEP9);
    display.print("AVG: ");
    display.print(RTC_avg_10s, 2);

    display.setCursor(offset + 120, cursor);
    display.print("Dist: ");
    display.print(Ublox.total_distance / 1000, 0);
  }
  // --- NO SAVE ---
  else {
    display.println("Going back to sleep");
  }

  drawChrome(offset, false);
  display.display(true);
}

/* =========================================================
 * Boot screen
 * ========================================================= */

void Boot_screen(void)
{

  display.firstPage();
  do {
    // Prepare framebuffer
    beginScreen();

    // Draw static chrome (RTC / boot mode)
    drawChrome(offset, true);

    // Header text
    display.setFont(Fonts::Body9);
    display.setCursor(offset, 14);
 

                // ------------------------------
                // LOW BATTERY PATH
                // ------------------------------
                if (RTC_voltage_bat < RTC_minimum_voltage_bat) {

                display.println("ESP-GPS sleeping");
                display.print("Go back to sleep...");

                display.setFont(Fonts::Body12);
                display.setCursor(offset, 60);
                display.printf("Voltage too low: %.2f", RTC_voltage_bat);

                display.setCursor(offset, 80);
                display.println("Please charge lipo!");

                display.setCursor(offset, 100);
                display.print(RTC_Sleep_txt);

                // Draw once, no follow-on redraw
                break;
                }


    display.setFullWindow();
    display.firstPage();
    do {
    display.fillScreen(GxEPD_WHITE);

    // Centered ESP logo


    } while (display.nextPage());

    // Short pause so logo is visible
    delay(1200);

    // ------------------------------
    // NORMAL BOOT PATH
    // ------------------------------
    //display.println("ESP-GPS booting");
    //display.print(E_paper_version);
    //display.println(SW_version);

    //sdCardInfo();

    //display.setCursor(offset, 102);
    //display.printf(
    //  "Logspace left : %d hour",
    //  storageLogTimeLeftMinutes() / 60
    //);

  } while (display.nextPage());
}


/* =========================================================
 * Sleep screen (RTC summary)
 * ========================================================= */

void Sleep_screen(int choice)
{
  // keep offset sane
  offset = constrain(offset, 1, 9);

  display.init();

  beginScreen();

  drawChrome(offset, true);

  /* ---------------- SIMPLE MODE ---------------- */
  if (choice == 0) {
    display.setFont(Fonts::Body18);

    display.setCursor(offset, 24);
    display.printf("Dist: %.0f", RTC_distance);

    display.setCursor(offset, 56);
    display.printf("AVG: %.2f", RTC_avg_10s);

    display.setCursor(offset, 88);
    display.printf("2s: %.2f", RTC_max_2s);

    display.display();
    return;
  }

  /* ---------------- DETAILED MODE ---------------- */

  constexpr int rowStep = 15;

  const int row1 = 15;
  const int row2 = row1 + rowStep;
  const int row3 = row2 + rowStep;
  const int row4 = row3 + rowStep;
  const int row5 = row4 + rowStep;
  const int row6 = row5 + rowStep;

  const int col1 = offset;
  const int col2 = offset + 34;
  const int col3 = offset + 90;
  const int col4 = offset + 146;

  display.setCursor(col1, 105);
  display.setFont(&SF_Distant_Galaxy9pt7b);
  display.print(RTC_Sleep_txt);

  drawSpeedUnitsFooter();

  // LEFT COLUMN
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

  // RIGHT COLUMN
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
