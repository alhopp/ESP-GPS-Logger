#include "System/watchdog_manager.h"

#include <Arduino.h>
#include <esp_task_wdt.h>

#include "Core/log.h"
#include "System/watchdog_config.h"

void watchdogInit()
{
  LOG_SYS("WDT", "configuring %ds", WDT_TIMEOUT);
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
}

void watchdogLoop()
{
  // Feed the loop task. Long-running worker code can still feed directly if it
  // owns a blocking operation.
  esp_task_wdt_reset();
}
