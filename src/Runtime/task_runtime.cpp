#include "Runtime/task_runtime.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Core/log.h"
#include "Runtime/task_display.h"
#include "Runtime/task_gps.h"

namespace {
constexpr uint32_t GPS_TASK_STACK = 4096;
constexpr UBaseType_t GPS_TASK_PRIO = 2;
constexpr BaseType_t GPS_TASK_CORE = 1;

constexpr uint32_t DISPLAY_TASK_STACK = 6096;
constexpr UBaseType_t DISPLAY_TASK_PRIO = 1;
constexpr BaseType_t DISPLAY_TASK_CORE = 0;

TaskHandle_t gpsTaskHandle = nullptr;
TaskHandle_t displayTaskHandle = nullptr;
}

bool startRuntimeTasks()
{
  gpsTaskHandle = nullptr;
  displayTaskHandle = nullptr;

  if (xTaskCreatePinnedToCore(
        gpsTask,
        "TaskGPS",
        GPS_TASK_STACK,
        nullptr,
        GPS_TASK_PRIO,
        &gpsTaskHandle,
        GPS_TASK_CORE) != pdPASS) {
    LOG_SYS("Task", "GPS task create failed");
    return false;
  }

  if (xTaskCreatePinnedToCore(
        displayTask,
        "TaskDisplay",
        DISPLAY_TASK_STACK,
        nullptr,
        DISPLAY_TASK_PRIO,
        &displayTaskHandle,
        DISPLAY_TASK_CORE) != pdPASS) {
    LOG_SYS("Task", "Display task create failed");
    if (gpsTaskHandle) {
      vTaskDelete(gpsTaskHandle);
      gpsTaskHandle = nullptr;
    }
    return false;
  }

  LOG_SYS("Task", "tasks started");
  if (gpsTaskHandle) LOG_SYS("Task", "GPS hw=%u", uxTaskGetStackHighWaterMark(gpsTaskHandle));
  if (displayTaskHandle) LOG_SYS("Task", "Display hw=%u", uxTaskGetStackHighWaterMark(displayTaskHandle));

  return true;
}
