#include "system_init.h"

#include <Arduino.h>
#include <SPI.h>
#include <sys/time.h>

#include "ESP_functions.h"

// ----------------------------------------------------
// Early system bring-up
// ----------------------------------------------------
void systemInitEarly()
{
  Serial.begin(115200);
  delay(1000); // allow USB/Serial to settle

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
