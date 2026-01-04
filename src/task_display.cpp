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
volatile bool display_dirty = true;

// Forward declaration of task handle
TaskHandle_t t2 = nullptr;

void screen_request_redraw()
{
  display_dirty = true;

  // Wake display task immediately
  if (t2) {
    xTaskNotifyGive(t2);
  }
}

// ============================================================================
// Display task
// ============================================================================
void taskTwo(void* parameter)
{
  // Capture our own task handle
  t2 = xTaskGetCurrentTaskHandle();

  LOG_TASK("Display", "task started");

  for (;;)
  {
    if (display_dirty)
    {
      display_dirty = false;

      // Safe full refresh baseline
      display.setFullWindow();
      display.firstPage();
      do {
        display.fillScreen(GxEPD_WHITE);
      } while (display.nextPage());

      const DrawFn draw = getDrawFnForMode(getMode());
      draw();
    }

    // Sleep until redraw requested or timeout
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500));
  }
}
