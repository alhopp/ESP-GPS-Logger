#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Display task handle (owned by task_display.cpp)
extern TaskHandle_t t2;

// Display task entry point
void taskTwo(void* parameter);
