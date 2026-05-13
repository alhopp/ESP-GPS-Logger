// ============================================================================
// app_supervisor.cpp
//
// Arduino loop() foreground supervisor. Time-critical GPS/display work lives in
// RTOS tasks; this loop only polls simple foreground services and mode policy.
// ============================================================================

#include "System/app_supervisor.h"

#include <Arduino.h>

#include "Core/magnet_input.h"
#include "Core/system_mode.h"
#include "System/watchdog_manager.h"

namespace {
constexpr uint32_t LOOP_DELAY_MS = 10;
}

void appSupervisorLoop()
{
  magnet_poll();
  watchdogLoop();
  systemModeLoop();
  delay(LOOP_DELAY_MS);
}
