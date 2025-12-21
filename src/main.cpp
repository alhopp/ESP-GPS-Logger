#include "Arduino.h"
#include "wifi_manager.h"
#include "storage_manager.h"
#include "config_manager.h"
#include "boot_manager.h"
#include "eeprom_manager.h"
#include "watchdog_manager.h"
#include "system_init.h"
#include "task_gps.h"
#include "task_display.h"
#include "system_phase.h"


static void startTasks();
extern RTC_DATA_ATTR int RTC_Sail_Logo;
extern RTC_DATA_ATTR char RTC_Sleep_txt[32];

bool sleep_mode=false;
extern bool reset_boot; 


void setup() {
  Serial.begin(115200);
  delay(200);

  PHASE(PH_BOOT_START, "systemInitEarly");
  systemInitEarly();

  PHASE(PH_STORAGE_INIT, "initStorage");
  initStorage();

  PHASE(PH_CONFIG_LOADED, "initConfig");
  initConfig();

  PHASE(PH_WIFI_INIT, "wifi_init");
  wifi_init();

  PHASE(PH_TASKS_CREATED, "startTasks");
  startTasks();

  PHASE(PH_RUNNING, "SETUP_DONE");
}



static void startTasks() {
  xTaskCreatePinnedToCore(taskOne, "TaskOne", 10000, NULL, 1, &t1, 1);
  xTaskCreatePinnedToCore(taskTwo, "TaskTwo", 20000, NULL, 1, &t2, 0);
}



void loop() {
  watchdogLoop();
  delay(100);
}


 
