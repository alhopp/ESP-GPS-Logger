#include "Core/sleep_control.h"

#include <esp_sleep.h>

#include "Core/input_pins.h"

bool sleep_woke_from_magnet()
{
  return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1;
}

void sleep_enter_from_magnet()
{
  esp_sleep_enable_ext1_wakeup(
    1ULL << MAGNET_PIN,
    ESP_EXT1_WAKEUP_ALL_LOW
  );
  esp_deep_sleep_start();
}
