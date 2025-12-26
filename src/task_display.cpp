#include "task_display.h"

#include <Arduino.h>

#include "E_paper.h"
#include "Fonts.h"
#include "Definitions.h"

#include "system_mode.h"
#include "screen_system.h"

// ----------------------------------------------------
// Task handle (owned here)
// ----------------------------------------------------
TaskHandle_t t2 = nullptr;

// ----------------------------------------------------
// Display task (MODE-AWARE, E-PAPER SAFE)
// ----------------------------------------------------
void taskTwo(void* parameter)
{
  LOG_TASK("Display", "task started");

  int value = 1;

  // Track last mode so we act only on transitions
  SystemMode lastMode = MODE_BOOT;

  // Partial update region for logging/test content
  const int X = 0;
  const int Y = 0;
  const int W = 200;
  const int H = 120;

  for (;;)
  {
    const SystemMode mode = getMode();

    // --------------------------------------------------
    // MODE CHANGE → CLEAR + DRAW ONCE
    // --------------------------------------------------
    if (mode != lastMode) {

      // Full refresh on mode transition
      display.setFullWindow();
      display.firstPage();
      do {
        display.fillScreen(GxEPD_WHITE);
      } while (display.nextPage());

      // Draw the new mode's screen ONCE
      if (mode == MODE_FIELD_CONFIG) {
        FieldAP_screen();        // BEACH MODE / WiFi AP
      }
      else if (mode == MODE_HOME) {
        HomeSTA_screen();        // STA MODE
      }
      else if (mode == MODE_SLEEP) {
        Sleep_screen(0);
      }

      lastMode = mode;
    }

    // --------------------------------------------------
    // HOLD MODES (NO REDRAW, NO FLICKER)
    // --------------------------------------------------
    if (mode == MODE_FIELD_CONFIG ||
        mode == MODE_HOME ||
        mode == MODE_SLEEP) {

      vTaskDelay(pdMS_TO_TICKS(500));
      continue;
    }

    // --------------------------------------------------
    // LOGGING MODE → TEST COUNTER (TEMPORARY)
    // --------------------------------------------------
    display.setPartialWindow(X, Y, W, H);
    display.firstPage();
    do {
      display.fillRect(X, Y, W, H, GxEPD_WHITE);
      display.setTextColor(GxEPD_BLACK);
      display.setFont(Fonts::SpeedXL);
      display.setCursor(X, Y + H);
      display.print(value);
    } while (display.nextPage());

    LOG_TASK("Display", "show %d", value);

    value++;
    if (value > 4) value = 1;

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
