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

#include "Runtime/Tasks/task_display.h"

#include <Arduino.h>

#include "esp_task_wdt.h"

#include "Core/log.h"

#include "Core/sleep_control.h"
#include "Core/system_mode.h"

#include "Display/E_paper.h"
#include "Display/screen_draw.h"
#include "Runtime/Display/display_redraw.h"

// ============================================================================
// Redraw signalling state
// ============================================================================

static volatile bool display_dirty = false;   // full refresh requested
static volatile bool partial_dirty = false;   // partial refresh requested

static DisplayWindow partialWindow = {0, 0, 0, 0};

static TaskHandle_t displayTaskHandle = nullptr;
static portMUX_TYPE redrawMux = portMUX_INITIALIZER_UNLOCKED;

namespace {
struct RefreshRequest {
  bool partial = false;
  DisplayWindow window = {0, 0, 0, 0};
};

bool hasPendingRefresh()
{
  taskENTER_CRITICAL(&redrawMux);
  const bool pending = display_dirty || partial_dirty;
  taskEXIT_CRITICAL(&redrawMux);
  return pending;
}

RefreshRequest takeRefreshRequest()
{
  RefreshRequest request;

  taskENTER_CRITICAL(&redrawMux);
  request.partial = partial_dirty && !display_dirty;
  request.window = partialWindow;

  display_dirty = false;
  partial_dirty = false;
  taskEXIT_CRITICAL(&redrawMux);

  return request;
}

void drawPage(const RefreshRequest& request, DrawFn draw)
{
  if (request.partial) {
    display.fillRect(
      request.window.x,
      request.window.y,
      request.window.w,
      request.window.h,
      GxEPD_WHITE
    );
  } else {
    display.fillScreen(GxEPD_WHITE);
  }

  if (draw) draw();

  esp_task_wdt_reset();
  vTaskDelay(1);
}

void renderRefresh(const RefreshRequest& request, DrawFn draw)
{
  if (request.partial) {
    display.setPartialWindow(
      request.window.x,
      request.window.y,
      request.window.w,
      request.window.h
    );
  } else {
    display.setFullWindow();
  }

  display.firstPage();
  do {
    drawPage(request, draw);
  } while (display.nextPage());
}

void enterDeepSleep(DrawFn draw)
{
  LOG_TASK("Display", "forcing final FULL refresh before deep sleep");

  // Reset any lingering partial-window state before the final full-screen draw.
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.setFullWindow();

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    if (draw) draw();
    esp_task_wdt_reset();
    vTaskDelay(1);
  } while (display.nextPage());

  delay(200);

  sleep_enter_from_magnet();
}
}

// ============================================================================
// Public redraw requests
// ============================================================================

// Request FULL redraw
void screen_request_redraw()
{
  taskENTER_CRITICAL(&redrawMux);
  display_dirty = true;
  taskEXIT_CRITICAL(&redrawMux);

  if (displayTaskHandle) xTaskNotifyGive(displayTaskHandle);
}

// Request PARTIAL redraw
void screen_request_partial(DisplayWindow window)
{
  taskENTER_CRITICAL(&redrawMux);
  partialWindow = window;
  partial_dirty = true;
  taskEXIT_CRITICAL(&redrawMux);

  if (displayTaskHandle) xTaskNotifyGive(displayTaskHandle);
}

// ============================================================================
// Display Task
// ============================================================================


void displayTask(void* parameter)
{
  displayTaskHandle = xTaskGetCurrentTaskHandle();
  LOG_TASK("Display", "task started");

  for (;;) {
    // Wait until someone asks for a redraw
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500));

    if (!hasPendingRefresh()) {
      continue;
    }

    const RefreshRequest request = takeRefreshRequest();
    const SystemMode mode = getMode();
    const DrawFn draw = getDrawFnForMode(mode);

    if (mode == MODE_SLEEP) {
      enterDeepSleep(draw);
      continue;
    }

    renderRefresh(request, draw);
  }
}
