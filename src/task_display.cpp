#include "task_display.h"

#include <Arduino.h>

#include "E_paper.h"
#include "Fonts.h"
#include "Definitions.h"

#include "system_mode.h"
#include "screen_system.h"

// -----------------------------------------------------------------------------
// DISPLAY REDRAW CONTROL
// -----------------------------------------------------------------------------
volatile bool display_dirty = true;   // start dirty → first draw happens

void screen_request_redraw()
{
  display_dirty = true;
}


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

  // Partial update region for logging/test content
  const int X = 0;
  const int Y = 0;
  const int W = 200;
  const int H = 120;

  for (;;)
  {
    const SystemMode mode = getMode();

    // --------------------------------------------------
    // REDRAW ON DEMAND (MODE CHANGE OR EXPLICIT REQUEST)
    // --------------------------------------------------
    if (display_dirty) {

      display_dirty = false;

      // Full refresh on mode change
      display.setFullWindow();
      display.firstPage();
      do {
        display.fillScreen(GxEPD_WHITE);
      } while (display.nextPage());

      // Draw current mode screen (authoritative mapping)
      DrawFn fn = getDrawFnForMode(mode);
      fn();
    }

    // --------------------------------------------------
    // HOLD MODES (STATIC UI, NO PERIODIC REDRAWS)
    // --------------------------------------------------
    if (mode == MODE_FIELD_CONFIG ||
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

    value++;
    if (value > 4) value = 1;

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
