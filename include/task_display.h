#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Display task handle (owned by task_display.cpp)
extern TaskHandle_t t2;

// Display task entry point
void taskTwo(void* parameter);

// Request a full screen redraw (async, display-task owned)
void screen_request_redraw();

// Request a partial screen redraw (async, display-task owned)
void screen_request_partial(int x, int y, int w, int h);
