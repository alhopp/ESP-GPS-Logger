#include "GPS/gps_power.h"

// Board-specific GPS power enable pins.
// Keep this file hardware-only: no UBX config, parsing, logging, or UI policy.

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>

#include "Core/board_pins.h"

void gps_power_on()
{
  pinMode(UBLOX_POWER1, OUTPUT);
  pinMode(UBLOX_POWER2, OUTPUT);
  pinMode(UBLOX_POWER3, OUTPUT);

  rtc_gpio_set_drive_capability(
    static_cast<gpio_num_t>(UBLOX_RTC_GPIO1),
    GPIO_DRIVE_CAP_3
  );

  rtc_gpio_set_drive_capability(
    static_cast<gpio_num_t>(UBLOX_RTC_GPIO2),
    GPIO_DRIVE_CAP_3
  );

  gpio_set_drive_capability(
    static_cast<gpio_num_t>(UBLOX_GPIO3),
    GPIO_DRIVE_CAP_3
  );

  delay(50);

  digitalWrite(UBLOX_POWER1, HIGH);
  digitalWrite(UBLOX_POWER2, HIGH);
  digitalWrite(UBLOX_POWER3, HIGH);

  delay(150);
}

void gps_power_off()
{
  digitalWrite(UBLOX_POWER1, LOW);
  digitalWrite(UBLOX_POWER2, LOW);
  digitalWrite(UBLOX_POWER3, LOW);
}
