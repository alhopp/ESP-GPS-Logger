#include "task_display.h"

#include <Arduino.h>

#include "E_paper.h"
#include "Fonts.h"
#include "Definitions.h"

#include "system_mode.h"
#include "screen_system.h"

// ============================================================================
// Display redraw control
// ============================================================================
// The display task is event-driven.
// A redraw occurs only when explicitly requested (mode change or UI event).
// This avoids unnecessary e-paper refreshes and preserves panel health.
//
volatile bool display_dirty = true;   // Start dirty → first draw always happens

void screen_request_redraw()
{
  display_dirty = true;
}

// ============================================================================
// Task handle (owned here)
// ============================================================================
TaskHandle_t t2 = nullptr;

// ============================================================================
// Display task (MODE-AWARE, E-PAPER SAFE)
// ============================================================================
// Responsibilities:
//  - Render the active screen based on SystemMode
//  - Perform full refreshes only when required
//  - Remain idle otherwise (no periodic redraws)
//
// Design principles:
//  - State-driven rendering
//  - No animations or timers in this task
//  - Predictable power usage
// ============================================================================
void taskTwo(void* parameter)
{
  LOG_TASK("Display", "task started");

  for (;;)
  {
    const SystemMode mode = getMode();

    // ------------------------------------------------------------------------
    // Redraw on demand only
    // ------------------------------------------------------------------------
    if (display_dirty)
    {
      display_dirty = false;

      // Full-screen refresh (safe baseline for all modes)
      display.setFullWindow();
      display.firstPage();
      do {
        display.fillScreen(GxEPD_WHITE);
      } while (display.nextPage());

      // Dispatch to the active mode's draw function
      const DrawFn draw = getDrawFnForMode(mode);
      draw();
    }

    // Light idle delay — keeps task responsive without wasting power
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
