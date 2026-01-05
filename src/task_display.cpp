

// ============================================================================
// Display task
// ============================================================================

#include "task_display.h"

#include <Arduino.h>

#include "E_paper.h"
#include "Fonts.h"
#include "Definitions.h"
#include "Layout.h"

#include "system_mode.h"
#include "screen_system.h"

// ============================================================================
// Display redraw signalling
//
// RULES:
// - display_dirty  → full refresh
// - partial_dirty  → partial refresh
// - display task owns ALL rendering + deep sleep
// ============================================================================

static volatile bool display_dirty  = true;
static volatile bool partial_dirty  = false;

static int partial_y = 0;
static int partial_h = 0;

// Display task handle (exported via task_display.h)
TaskHandle_t t2 = nullptr;

// -----------------------------------------------------------------------------
// Request FULL redraw
// -----------------------------------------------------------------------------
void screen_request_redraw()
{
  display_dirty = true;

  if (t2) {
    xTaskNotifyGive(t2);
  }
}

// -----------------------------------------------------------------------------
// Request PARTIAL redraw (caller defines dirty band)
// -----------------------------------------------------------------------------
void screen_request_partial(int y, int h)
{
  partial_y     = y;
  partial_h     = h;
  partial_dirty = true;

  if (t2) {
    xTaskNotifyGive(t2);
  }
}

// ============================================================================
// Display Task
// ============================================================================

void taskTwo(void* parameter)
{
  // Publish task handle
  t2 = xTaskGetCurrentTaskHandle();

  LOG_TASK("Display", "task started");

  for (;;)
  {
    // -------------------------------------------------------------------------
    // Wake only when something changed
    // -------------------------------------------------------------------------
    if (display_dirty || partial_dirty)
    {
      // Decide refresh type
      const bool doPartial = partial_dirty && !display_dirty;

      // Snapshot partial region early (avoid races)
      const int py = partial_y;
      const int ph = partial_h;

      // Clear latches
      display_dirty = false;
      partial_dirty = false;

      // Resolve current mode + draw function
      const SystemMode mode = getMode();
      const DrawFn     draw = getDrawFnForMode(mode);

      // -----------------------------------------------------------------------
      // Select refresh window
      // -----------------------------------------------------------------------
      if (doPartial) {
        display.setPartialWindow(0,40,250,94);
      } else {
        display.setFullWindow();
      }


      // -----------------------------------------------------------------------
      // Render loop (display task owns paging)
      // -----------------------------------------------------------------------
      display.firstPage();
      do {
        if (!doPartial) {
          // Full refresh clears everything
          display.fillScreen(GxEPD_WHITE);
        } else {
          // Partial refresh clears ONLY dirty band
          display.fillRect(0,40,250,94,GxEPD_WHITE);
        }

        // Draw active screen
        if (draw) {
          draw();
        }

      } while (display.nextPage());

      // -----------------------------------------------------------------------
      // FINAL ACTION: SLEEP TRANSITION (owned HERE)
      // -----------------------------------------------------------------------
      if (mode == MODE_SLEEP) {

        LOG_TASK("Display", "final refresh complete → deep sleep");

        // Let EPD waveform settle
        delay(200);

        // Ensure magnet released before sleeping
        while (digitalRead(MAGNET_PIN) == LOW) {
          delay(10);
        }

        // Wake on magnet / hall sensor
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 0);

        // No return
        esp_deep_sleep_start();
      }
    }

    // -------------------------------------------------------------------------
    // Idle until notified (safety timeout prevents deadlock)
    // -------------------------------------------------------------------------
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500));
  }
}
