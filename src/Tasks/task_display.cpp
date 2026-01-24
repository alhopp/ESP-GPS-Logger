// ============================================================================
// task_display.cpp
//
// Display task:
// - Owns ALL rendering (full + partial refresh)
// - Owns ALL display timing and paging
// - Owns deep sleep entry and wake configuration
//
// RULES:
// - No drawing outside this file
// - No sleep entry outside this file
// - Other code may ONLY signal redraw intent
// ============================================================================

#include "tasks/task_display.h"

#include <Arduino.h>

#include "esp_sleep.h"
#include "esp_task_wdt.h"

#include "Fonts.h"
#include "Core/Definitions.h"
#include "Layout.h"
#include "Core/Globals.h"

#include "core/system_mode.h"

#include "Display/E_paper.h"
#include "Display/screen_draw.h"
#include "Display/Screens/screen_system.h"

#include "Core/magnet_input.h"

// ============================================================================
// Redraw signalling state
// ============================================================================

static volatile bool display_dirty = false;   // full refresh requested
static volatile bool partial_dirty = false;   // partial refresh requested

static int partial_x = 0;
static int partial_y = 0;
static int partial_w = 0;
static int partial_h = 0;

// Display task handle (exported)
TaskHandle_t t2 = nullptr;

// ============================================================================
// Public redraw requests
// ============================================================================

// Request FULL redraw
void screen_request_redraw()
{
  display_dirty = true;
  if (t2) xTaskNotifyGive(t2);
}

// Request PARTIAL redraw
void screen_request_partial(int x, int y, int w, int h)
{
  partial_x = x; partial_y = y; partial_w = w; partial_h = h;
  partial_dirty = true;
  if (t2) xTaskNotifyGive(t2);
}

// ============================================================================
// Display Task
// ============================================================================


void taskTwo(void* parameter)
{
  t2 = xTaskGetCurrentTaskHandle();
  LOG_TASK("Display", "task started");

  for (;;)
  {
    // Wait until someone asks for a redraw
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500));

    if (!display_dirty && !partial_dirty)
      continue;

    const bool doPartial = partial_dirty && !display_dirty;
    const int px = partial_x, py = partial_y, pw = partial_w, ph = partial_h;

    display_dirty  = false;
    partial_dirty  = false;

    const SystemMode mode = getMode();
    const DrawFn draw = getDrawFnForMode(mode);

    // -----------------------------------------------------------------------
    // Refresh policy (intent-driven, not mode-driven)
    // -----------------------------------------------------------------------
    if (doPartial) {
      display.setPartialWindow(px, py, pw, ph);
    } else {
      display.setFullWindow();
    }

    display.firstPage();
    do {
      if (doPartial)
        display.fillRect(px, py, pw, ph, GxEPD_WHITE);
      else
        display.fillScreen(GxEPD_WHITE);

      if (draw) draw();

      esp_task_wdt_reset();
      vTaskDelay(1);

    } while (display.nextPage());

    // -----------------------------------------------------------------------
    // Deep sleep handling (unchanged)
    // -----------------------------------------------------------------------
    if (mode == MODE_SLEEP) {
      LOG_TASK("Display", "final refresh complete → deep sleep");
      delay(200);

      esp_sleep_enable_ext1_wakeup(
        1ULL << MAGNET_PIN,
        ESP_EXT1_WAKEUP_ALL_LOW
      );
      esp_deep_sleep_start();
    }
  }
}
