#include "Runtime/Tasks/task_runtime.h"

// ============================================================================
// Runtime task launcher
//
// Creates the GPS and display tasks on their intended ESP32 cores. If display
// task creation fails after the GPS task was started, the GPS task is deleted so
// startup does not continue with a half-running runtime.
// ============================================================================

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Core/log.h"
#include "Runtime/Tasks/task_display.h"
#include "Runtime/Tasks/task_gps.h"

namespace {
constexpr uint32_t GPS_TASK_STACK = 4096;
constexpr UBaseType_t GPS_TASK_PRIO = 2;
constexpr BaseType_t GPS_TASK_CORE = 1;

constexpr uint32_t DISPLAY_TASK_STACK = 6096;
constexpr UBaseType_t DISPLAY_TASK_PRIO = 1;
constexpr BaseType_t DISPLAY_TASK_CORE = 0;

TaskHandle_t gpsRuntimeTaskHandle = nullptr;
TaskHandle_t displayRuntimeTaskHandle = nullptr;

struct RuntimeTaskSpec {
  TaskFunction_t entry;
  const char* name;
  uint32_t stack;
  UBaseType_t priority;
  BaseType_t core;
  TaskHandle_t* handle;
};

bool createRuntimeTask(const RuntimeTaskSpec& spec)
{
  *spec.handle = nullptr;
  return xTaskCreatePinnedToCore(
    spec.entry,
    spec.name,
    spec.stack,
    nullptr,
    spec.priority,
    spec.handle,
    spec.core
  ) == pdPASS;
}

void stopTask(TaskHandle_t& handle)
{
  if (!handle) return;
  vTaskDelete(handle);
  handle = nullptr;
}

void logTaskHighWaterMark(const char* name, TaskHandle_t handle)
{
  if (!handle) return;
  LOG_SYS("Task", "%s hw=%u", name, uxTaskGetStackHighWaterMark(handle));
}
}

bool startRuntimeTasks()
{
  gpsRuntimeTaskHandle = nullptr;
  displayRuntimeTaskHandle = nullptr;

  const RuntimeTaskSpec gpsSpec = {
        gpsTask,
        "TaskGPS",
        GPS_TASK_STACK,
        GPS_TASK_PRIO,
        GPS_TASK_CORE,
        &gpsRuntimeTaskHandle
  };
  const RuntimeTaskSpec displaySpec = {
        displayTask,
        "TaskDisplay",
        DISPLAY_TASK_STACK,
        DISPLAY_TASK_PRIO,
        DISPLAY_TASK_CORE,
        &displayRuntimeTaskHandle
  };

  if (!createRuntimeTask(gpsSpec)) {
    LOG_SYS("Task", "GPS task create failed");
    return false;
  }

  if (!createRuntimeTask(displaySpec)) {
    LOG_SYS("Task", "Display task create failed");
    stopTask(gpsRuntimeTaskHandle);
    return false;
  }

  LOG_SYS("Task", "tasks started");
  logTaskHighWaterMark("GPS", gpsRuntimeTaskHandle);
  logTaskHighWaterMark("Display", displayRuntimeTaskHandle);

  return true;
}
