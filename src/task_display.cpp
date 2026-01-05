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
// - the display task owns all rendering
// ============================================================================

static volatile bool display_dirty = true;

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

// ============================================================================
// Display task
// ============================================================================

void taskTwo(void* parameter)
{
  // Publish our task handle for notifications
  t2 = xTaskGetCurrentTaskHandle();

  LOG_TASK("Display", "task started");

  for (;;)
  {
  // -----------------------------------------------------------------------------
  // Redraw handling
  //
  // This block is entered ONLY when a redraw has been requested.
  // The request can come from:
  //  - a system mode change (LOGGING / CONFIG / SLEEP / BOOT)
  //  - an explicit screen_request_redraw() call
  // -----------------------------------------------------------------------------
  if (display_dirty)
  {
    // Clear the redraw latch immediately.
    display_dirty = false;

    // ---------------------------------------------------------------------------
    // Determine WHAT screen to draw
    // ---------------------------------------------------------------------------
    const SystemMode mode = getMode();              // Current system state

    // -----------------------------------------------------------------------------
    // The display task uses a SINGLE draw loop.
    // What changes is the draw() function pointer selected at runtime.
    //
    // Depending on the current SystemMode, the draw function pointer resolves to:
    //
    //   SystemMode           → draw() points to
    //   ------------------------------------------------
    //   MODE_LOGGING         → draw_LOGGING()
    //   MODE_WIFI_SOFT_AP    → draw_FIELD_CFG()
    //   MODE_WAIT_SATS       → draw_WAIT_SATS()
    //   MODE_SLEEP           → draw_SLEEP()
    //
    // The draw loop itself never changes.
    // Only the function that draw() invokes is different.
    // -----------------------------------------------------------------------------


    const DrawFn     draw = getDrawFnForMode(mode); // Mode → draw function mapping

    // ---------------------------------------------------------------------------
    // Begin a full e-paper refresh
    // ---------------------------------------------------------------------------
    display.setFullWindow();   // Target the entire screen
    display.firstPage();

    do {
      // Clear the current page buffer to a known background.
      // This ensures no ghosting or leftover pixels.
      display.fillScreen(GxEPD_WHITE);

      // -------------------------------------------------------------------------
      // Draw the active screen
      //
      // The draw function:
      //  - renders the UI for the current mode
      // -------------------------------------------------------------------------
      if (draw) {
        draw();
      }

    // Commit the current page and advance until the full screen is updated
    } while (display.nextPage());
  }

    // -------------------------------------------------------------------------
    // Sleep until:
    //  - a redraw is requested, or
    //  - timeout (acts as a safety wake)
    // -------------------------------------------------------------------------
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500));
  }
}
