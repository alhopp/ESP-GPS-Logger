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
#include "E_paper.h"
#include "screen_system.h"

static void startTasks();
extern RTC_DATA_ATTR int RTC_Sail_Logo;
extern RTC_DATA_ATTR char RTC_Sleep_txt[32];

bool sleep_mode=false;
extern bool reset_boot; 



void setup() {

  //display.init();
  
  systemInitEarly();

  initStorage();

  initConfig();

  wifi_init();

  startTasks();

}



static void startTasks() {
  xTaskCreatePinnedToCore(taskOne, "TaskOne", 10000, NULL, 1, &t1, 1);
  xTaskCreatePinnedToCore(taskTwo, "TaskTwo", 20000, NULL, 1, &t2, 0);
}



void loop() {
  watchdogLoop();
  delay(100);
}


 
