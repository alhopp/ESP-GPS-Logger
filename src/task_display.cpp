#include "task_display.h"

#include <Arduino.h>

#include "Ublox.h"
#include "SD_card.h"
#include "ESP_functions.h"
#include "E_paper.h"
#include "Fonts.h"


// ----------------------------------------------------
// Task handle (owned here)
// ----------------------------------------------------
TaskHandle_t t2 = nullptr;

// ----------------------------------------------------
// External state used by display task
// ----------------------------------------------------
extern bool sleep_mode;

// ----------------------------------------------------
// Internal helpers
// ----------------------------------------------------
static void handleSleep();
static void handleLowBattery();
static void updateDisplay();

// ----------------------------------------------------
// Display / UI task
// ----------------------------------------------------
void taskTwo(void* parameter)
{


   Serial.println("[TASK2] display task entered");

      display.init(115200);
        display.setRotation(1);//

        display.firstPage();
        do {
            display.fillScreen(GxEPD_WHITE);
            display.setTextColor(GxEPD_BLACK);
            display.setFont(&FreeSansBold18pt7b);
            display.setCursor(10, 40);
            display.print("HELLO");
       } while (display.nextPage());

        Off_screen(1);

  Serial.println("[TASK2] display refresh done");

  // park task forever
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
    Serial.println("[TASK2] display refresh done");

    // park task forever
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }


  while (true) {

    // ------------------------------------------------
    // Watchdog heartbeat
    // ------------------------------------------------
    wdt_task1 = millis();

    // ------------------------------------------------
    // Auto-cycle stat screens
    // ------------------------------------------------
    if (config.Stat_screens_time != 0) {
      stat_count++;
    }
    if (stat_count > config.screen_count) {
      stat_count = 0;
    }

    Update_bat();

    // ------------------------------------------------
    // Battery supervision
    // ------------------------------------------------
    if (RTC_voltage_bat < RTC_minimum_voltage_bat) {
      low_bat_count++;
    } else {
      low_bat_count = 0;
    }

    // ------------------------------------------------
    // Power-down paths
    // ------------------------------------------------
    if (sleep_mode) {
      handleSleep();
    }
    else if (low_bat_count > 10) {
      handleLowBattery();
    }
    else {
      updateDisplay();
    }
  }
}

static void handleSleep()
{
  Ublox_off();
  Off_screen(RTC_OFF_screen);
  Serial.println("RTC_OFF_screen");
  delay(2000);
  Shut_down();
  delay(100);
  vTaskDelete(nullptr);
}

static void handleLowBattery()
{
  sleep_mode = true;

  char tekst[32];
  sprintf(tekst, "Shutdown low bat @ %.1f V\n", RTC_minimum_voltage_bat);
  logERR(tekst);

  Off_screen(2); // shutdown low battery screen
  delay(2000);
  Shut_down();
  delay(100);
  vTaskDelete(nullptr);
}

static void updateDisplay()
{
  if (millis() < 2000) {
    Update_screen(BOOT_SCREEN);
  }
  else if (trouble_screen) {
    Update_screen(TROUBLE);
  }
  else if (!GPS_Signal_OK) {
    Update_screen(WIFI_ON);
  }
  else if (!Time_Set_OK) {
    Update_screen(WIFI_ON);
  }
#if defined(GPIO12_ACTIF)
  else if (Short_push12.long_pulse) {
    Update_screen(config.gpio12_screen[GPIO12_screen]);
  }
#endif
  else if ((gps_speed / 1000.0f < config.stat_speed) && !Field_choice) {
    Update_screen(config.stat_screen[stat_count]);
  }
  else {
    Update_screen(SPEED);
    if (config.Stat_screens_time != 0) stat_count = 0;
  }
}
