#include <Arduino.h>
#include "system_init.h"
#include <SPI.h>
#include <sys/time.h>
#include "ESP_functions.h"
#include "config_manager.h"

// ----------------------------------------------------
// Early system bring-up
// ----------------------------------------------------
void systemInitEarly()
{
  Serial.begin(115200);
  
  uint32_t t0 = millis();
   while (!Serial && millis() - t0 < 2000) {
   delay(100);
  }

  Serial.println(F("System initializing..."));
}

// ----------------------------------------------------
// SPI + system time initialisation
// ----------------------------------------------------
void systemInitSPIAndTime()
{
  // Pre-fill battery ADC filter
  analog_mean = analogRead(PIN_BAT);

  // Initialise SPI bus
  SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI, ELINK_SS);

  // Reset system time (epoch = 0)
  struct timeval tv = {
    .tv_sec  = 0,
    .tv_usec = 0
  };
  settimeofday(&tv, nullptr);
}
