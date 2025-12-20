#include "watchdog_manager.h"
#include <Arduino.h>
#include <esp_task_wdt.h>
#include "ESP_functions.h"

void watchdogInit() {
  Serial.println("Configuring WDT...");
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
}

void watchdogLoop() {
  int wdt_task0_duration = millis() - wdt_task0;
  int wdt_task1_duration = millis() - wdt_task1;
  int task_timeout = (WDT_TIMEOUT - 1) * 1000;

  if ((wdt_task0_duration < task_timeout) &&
      (wdt_task1_duration < task_timeout)) {
    feedTheDog_Task0();
    feedTheDog_Task1();
  }

  if ((wdt_task0_duration > task_timeout) &&
      downloading_file &&
      (max_count_wdt_task0 < MAX_COUNT_WDT_TASK0)) {

    max_count_wdt_task0++;
    feedTheDog_Task0();
    wdt_task0 = millis();

    Serial.println("Extend watchdog timeout due long download");
  }

  if ((wdt_task0_duration > task_timeout) && !downloading_file)
    Serial.println("Watchdog task0 triggered");

  if (wdt_task1_duration > task_timeout)
    Serial.println("Watchdog task1 triggered");
}
