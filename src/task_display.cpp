#include "task_display.h"

#include <Arduino.h>

#include "E_paper.h"
#include "Fonts.h"
#include "Definitions.h"

#include "system_mode.h"
#include "screen_system.h"

// ============================================================================
// Display redraw signalling
//
// - display_dirty is the single redraw latch
// - screen_request_redraw() is the ONLY external entry point
// - the display task owns all rendering and final sleep
// ============================================================================

static volatile bool display_dirty = true;
static volatile bool partial_dirty = false;
static int partial_y = 0;
static int partial_h = 0;


// Display task handle (published via task_display.h)
TaskHandle_t t2 = nullptr;

// -----------------------------------------------------------------------------
// Request a full display redraw
// -----------------------------------------------------------------------------
void screen_request_redraw()
{
  display_dirty = true;

  // Wake the display task immediately if running
  if (t2) {
    xTaskNotifyGive(t2);
  }
}

void screen_request_partial(int y, int h)
{
  partial_y = y;
  partial_h = h;
  partial_dirty = true;

  if (t2) {
    xTaskNotifyGive(t2);
  }
}



// ============================================================================
// Display task
// ============================================================================

void taskTwo(void* parameter)
{
  t2 = xTaskGetCurrentTaskHandle();
  LOG_TASK("Display", "task started");

  for (;;)
  {
    if (display_dirty || partial_dirty)
    {
      const bool doPartial = partial_dirty && !display_dirty;

      display_dirty = false;
      partial_dirty = false;

      const SystemMode mode = getMode();
      const DrawFn draw     = getDrawFnForMode(mode);

      // -----------------------------
      // Select refresh window
      // -----------------------------
      if (doPartial) {
        display.setPartialWindow(
          0,
          partial_y,
          display.width(),
          partial_h
        );
      } else {
        display.setFullWindow();
      }

      display.firstPage();
      do {
        display.fillScreen(GxEPD_WHITE);
        if (draw) draw();
      } while (display.nextPage());

      // -----------------------------
      // FINAL ACTION: sleep transition
      // -----------------------------
      if (mode == MODE_SLEEP) {

        LOG_TASK("Display", "final refresh complete → deep sleep");
        delay(200);

        while (digitalRead(MAGNET_PIN) == LOW) {
          delay(10);
        }

        esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 0);
        esp_deep_sleep_start();
      }
    }

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500));
  }
}
