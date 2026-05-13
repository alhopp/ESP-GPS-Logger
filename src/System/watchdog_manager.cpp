// ============================================================================
// watchdog_manager.cpp
//
// Foreground watchdog feed used by the Arduino loop supervisor.
// ============================================================================

#include "System/watchdog_manager.h"

#include <esp_task_wdt.h>

void watchdogLoop()
{
  // Feed the loop task. Long-running worker code can still feed directly if it
  // owns a blocking operation.
  esp_task_wdt_reset();
}
