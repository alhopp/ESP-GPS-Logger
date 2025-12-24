#include "gps_manager.h"
#include "Ublox.h"
#include "config_manager.h"
#include "Definitions.h"
#include <Arduino.h>

// --------------------------------------------------
// Internal state
// --------------------------------------------------
static GpsChip detectedChip = GpsChip::UNKNOWN;

// --------------------------------------------------
// Public API
// --------------------------------------------------
void initGPS()
{
  Serial.println("[GPS    ] Init         : starting");

  Ublox_on();              
  Ublox_serial2(300);         

  delay(400);                 

  detectedChip = static_cast<GpsChip>(Auto_detect_ublox());

  switch (detectedChip) {
    case GpsChip::UBLOX_M8:
    case GpsChip::UBLOX_M9:
      Serial.println("[GPS    ] Chip         : u-blox M8/M9");
      Init_ublox();
      Set_rate_ublox(config.sample_rate);
      break;

    case GpsChip::UBLOX_M10:
      Serial.println("[GPS    ] Chip         : u-blox M10");
      Init_ubloxM10();

      if (Check_M10_nav_rate() == 0) {
        Serial.println("[GPS    ] Nav rate     : enabling high-rate");
        Set_M10_high_nav_rate();
      }

      Set_rate_ubloxM10(config.sample_rate);
      break;

    default:
      Serial.println("[GPS    ] Init         : no GPS detected");
      break;
  }
}

void Ublox_on(){
  pinMode(UBLOX_POWER1, OUTPUT);//Power beitian //default drive strength 2, only 2.7V @ ublox gps
  pinMode(UBLOX_POWER2, OUTPUT);//Power beitian
  pinMode(UBLOX_POWER3, OUTPUT);//Power beitian
  rtc_gpio_set_drive_capability(UBLOX_RTC_GPIO1,GPIO_DRIVE_CAP_3);// https://www.esp32.com/viewtopic.php?t=5840
  rtc_gpio_set_drive_capability(UBLOX_RTC_GPIO2,GPIO_DRIVE_CAP_3);//3.0V @ ublox gps current 50 mA
  gpio_set_drive_capability(UBLOX_GPIO3,GPIO_DRIVE_CAP_3);//rtc_gpio_ necessary, if not no output on RTC_pins 25 en 26, 13/3/2022
  delay(50);
  digitalWrite(UBLOX_POWER1, HIGH); 
  digitalWrite(UBLOX_POWER2, HIGH);
  digitalWrite(UBLOX_POWER3, HIGH);
  delay(100);
}

void Ublox_off(){
  digitalWrite(UBLOX_POWER1, LOW);
  digitalWrite(UBLOX_POWER2, LOW);
  digitalWrite(UBLOX_POWER3, LOW);
}

