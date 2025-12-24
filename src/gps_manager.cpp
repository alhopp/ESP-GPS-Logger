// -----------------------------------------------------------------------------
// gps_manager.cpp
//
// Minimal, SAFE GPS bring-up for ESP32 + u-blox
// - Powers GPS
// - Scans baud rates
// - Confirms GPS presence via UBX-MON-VER
// - No UBX config, no parsing (yet)
// -----------------------------------------------------------------------------

#include "gps_manager.h"

#include <Arduino.h>
#include <HardwareSerial.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>

#include "Definitions.h"
#include "ESP_functions.h"

// -----------------------------------------------------------------------------
// SERIAL + PINS
// -----------------------------------------------------------------------------
static HardwareSerial GPSSerial(2);

#define GPS_RX_PIN  32
#define GPS_TX_PIN  33


// -----------------------------------------------------------------------------
// POWER CONTROL
// -----------------------------------------------------------------------------
static void gps_power_on()
{
  pinMode(UBLOX_POWER1, OUTPUT);
  pinMode(UBLOX_POWER2, OUTPUT);
  pinMode(UBLOX_POWER3, OUTPUT);

  rtc_gpio_set_drive_capability(UBLOX_RTC_GPIO1, GPIO_DRIVE_CAP_3);
  rtc_gpio_set_drive_capability(UBLOX_RTC_GPIO2, GPIO_DRIVE_CAP_3);
  gpio_set_drive_capability(UBLOX_GPIO3, GPIO_DRIVE_CAP_3);

  delay(50);

  digitalWrite(UBLOX_POWER1, HIGH);
  digitalWrite(UBLOX_POWER2, HIGH);
  digitalWrite(UBLOX_POWER3, HIGH);

  delay(150);
}

static void gps_power_off()
{
  digitalWrite(UBLOX_POWER1, LOW);
  digitalWrite(UBLOX_POWER2, LOW);
  digitalWrite(UBLOX_POWER3, LOW);
}

// -----------------------------------------------------------------------------
// HELPERS
// -----------------------------------------------------------------------------
static bool probe_gps(uint32_t baud)
{
  GPSSerial.begin(baud, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  delay(120);

  // Flush startup noise
  while (GPSSerial.available()) GPSSerial.read();

  // Send MON-VER poll (reuse global UBX_MON_VER)
    GPSSerial.write(
    (const uint8_t*)UBX_MON_VER,
    sizeof(UBX_MON_VER)
    );

  GPSSerial.flush();

  uint32_t start = millis();
  while (millis() - start < 300) {
    if (GPSSerial.available()) {
      return true;   // Any response = GPS alive
    }
  }

  return false;
}

// -----------------------------------------------------------------------------
// PUBLIC API
// -----------------------------------------------------------------------------
bool initGPS()
{
  LOG_GPS("Init", "starting");

  gps_power_on();

  LOG_GPS("Detect", "baud scan");

  if (probe_gps(9600)) {
    LOG_GPS("Detect", "baud=9600");
    return true;
  }

  if (probe_gps(38400)) {
    LOG_GPS("Detect", "baud=38400");
    return true;
  }

  if (probe_gps(115200)) {
    LOG_GPS("Detect", "baud=115200");
    return true;
  }

  LOG_GPS("Init", "no GPS detected");
  return false;
}

void gps_shutdown()
{
  LOG_GPS("Power", "off");
  gps_power_off();
}

// -----------------------------------------------------------------------------
// LEGACY COMPATIBILITY WRAPPERS
// -----------------------------------------------------------------------------
void Ublox_on()
{
  gps_power_on();
}

void Ublox_off()
{
  gps_power_off();
}
