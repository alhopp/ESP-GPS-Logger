#include "task_display.h"

#include <Arduino.h>

#include "E_paper.h"
#include "Fonts.h"
#include "Definitions.h"

// ----------------------------------------------------
// Task handle (owned here)
// ----------------------------------------------------
TaskHandle_t t2 = nullptr;

// ----------------------------------------------------
// Display task (TEST MODE)
// ----------------------------------------------------
void taskTwo(void* parameter)
{
  LOG_TASK("Display", "test mode start");

  int value = 1;
  bool first = true;

  // Define the area that will change
  const int X = 0;
  const int Y = 0;
  const int W = 200;
  const int H = 120;

  for (;;)
  {
    if (first)
    {
      // -------------------------------------------------
      // ONE-TIME full refresh to initialise panel
      // -------------------------------------------------
      display.setFullWindow();
      display.firstPage();
      do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);

        display.setFont(Fonts::SpeedXL);
        display.setCursor(X, Y + H);
        display.print(value);
      } while (display.nextPage());

      first = false;
    }
    else
    {
      // -------------------------------------------------
      // PARTIAL refresh only (NO FLICKER)
      // -------------------------------------------------
      display.setPartialWindow(X, Y, W, H);
      display.firstPage();
      do {
        // IMPORTANT: clear only the partial area
        display.fillRect(X, Y, W, H, GxEPD_WHITE);

        display.setTextColor(GxEPD_BLACK);
        display.setFont(Fonts::SpeedXL);
        display.setCursor(X, Y + H);
        display.print(value);
      } while (display.nextPage());
    }

    LOG_TASK("Display", "show %d", value);

    value++;
    if (value > 4) value = 1;

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

/* ========================================================================== */
/* ======================= ORIGINAL CODE (PARKED) =========================== */
/* ========================================================================== */

/*

// ----------------------------------------------------
// External state used by display task
// ----------------------------------------------------
extern bool sleep_mode;

// ----------------------------------------------------
// Internal helpers
// ----------------------------------------------------
static void handleSleep();
static void handleLowBattery();
static void updateDisplay();

// ----------------------------------------------------
// Display / UI task (ORIGINAL)
// ----------------------------------------------------
void taskTwo(void* parameter)
{
  Serial.println("[TASK2] display task entered");

  Boot_screen();

  while (true) {

    wdt_task1 = millis();

    if (config.Stat_screens_time != 0) {
      stat_count++;
    }
    if (stat_count > config.screen_count) {
      stat_count = 0;
    }

    Update_bat();

    if (RTC_voltage_bat < RTC_minimum_voltage_bat) {
      low_bat_count++;
    } else {
      low_bat_count = 0;
    }

    if (sleep_mode) {
      handleSleep();
    }
    else if (low_bat_count > 10) {
      handleLowBattery();
    }
    else {
      updateDisplay();
    }
  }
}

*/

