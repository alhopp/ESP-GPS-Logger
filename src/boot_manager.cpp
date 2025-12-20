#include "boot_manager.h"

#include <Arduino.h>
#include <cstring>

#include "ESP_functions.h"
#include "E_paper.h"

// ----------------------------------------------------
// External state used during boot checks
// ----------------------------------------------------
extern float RTC_voltage_bat;
extern float RTC_minimum_voltage_bat;
extern int   RTC_OFF_screen;

extern RTC_DATA_ATTR char RTC_Sleep_txt[32];
extern bool reset_boot;

// ----------------------------------------------------
// Internal helpers
// ----------------------------------------------------
static void shutdownWithMessage(const char* msg)
{
  RTC_OFF_screen = 1;
  strncpy(RTC_Sleep_txt, msg, sizeof(RTC_Sleep_txt) - 1);
  RTC_Sleep_txt[sizeof(RTC_Sleep_txt) - 1] = '\0';
  Shut_down();
}

// ----------------------------------------------------
// Public API
// ----------------------------------------------------
void initBootChecks()
{
  Boot_screen();

  // Low battery check
  if (RTC_voltage_bat < RTC_minimum_voltage_bat) {
    logERR("Shutdown low bat");
    shutdownWithMessage("Shut down Low Bat !");
    return; // defensive: should not continue after shutdown
  }

  // Reset-triggered shutdown
  if (reset_boot) {
    shutdownWithMessage("Shutdown after reset!");
    return;
  }

  // Normal boot continuation
  setCpuFrequencyMhz(240);
  Update_screen(BOOT_SCREEN);

  Serial.print("Actual CPU freq before Wifi.begin(): ");
  Serial.println(getCpuFrequencyMhz());
}
