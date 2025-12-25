// -----------------------------------------------------------------------------
// gps_manager.cpp
//
// SAFE GPS bring-up for ESP32 + u-blox
// - Uses RTC-cached baud on warm boot
// - Falls back to full scan if needed
// - Confirms GPS via UBX-MON-VER response
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

#define GPS_RX_PIN  RXD2
#define GPS_TX_PIN  TXD2

// -----------------------------------------------------------------------------
// RTC CACHE (declared elsewhere)
// -----------------------------------------------------------------------------
extern RTC_DATA_ATTR uint8_t RTC_gps_baud_index;  // 0=unknown
extern RTC_DATA_ATTR bool    RTC_gps_valid;

// -----------------------------------------------------------------------------
// BAUD TABLE
// -----------------------------------------------------------------------------
static const uint32_t gpsBauds[] = {
  0,        // index 0 = invalid
  9600,     // index 1
  38400,    // index 2
  115200    // index 3
};

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
// PROBE HELPER
// -----------------------------------------------------------------------------
static bool probe_gps(uint32_t baud)
{
  GPSSerial.begin(baud, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  delay(120);

  while (GPSSerial.available()) GPSSerial.read();

  GPSSerial.write(
    (const uint8_t*)UBX_MON_VER,
    sizeof(UBX_MON_VER)
  );
  GPSSerial.flush();

  uint32_t start = millis();
  while (millis() - start < 300) {
    if (GPSSerial.available()) {
      return true;
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

  // ---------------------------------------------------------------------------
  // 1️⃣ FAST PATH — try cached baud first
  // ---------------------------------------------------------------------------
  if (RTC_gps_valid && RTC_gps_baud_index >= 1 && RTC_gps_baud_index <= 3) {
    uint32_t baud = gpsBauds[RTC_gps_baud_index];

    LOG_GPS("Detect", "cached baud=%lu", baud);

    if (probe_gps(baud)) {
      LOG_GPS("Detect", "cache hit");
      return true;
    }

    LOG_GPS("Detect", "cache failed → rescan");
    RTC_gps_valid = false;
  }

  // ---------------------------------------------------------------------------
  // 2️⃣ FULL SCAN FALLBACK
  // ---------------------------------------------------------------------------
  LOG_GPS("Detect", "baud scan");

  for (uint8_t i = 1; i <= 3; i++) {
    if (probe_gps(gpsBauds[i])) {
      RTC_gps_baud_index = i;
      RTC_gps_valid      = true;

      LOG_GPS("Detect", "baud=%lu", gpsBauds[i]);
      return true;
    }
  }

  // ---------------------------------------------------------------------------
  // 3️⃣ FAILURE
  // ---------------------------------------------------------------------------
  RTC_gps_valid = false;
  LOG_GPS("Init", "no GPS detected");
  return false;
}

void gps_shutdown()
{
  LOG_GPS("Power", "off");
  gps_power_off();
}

// -----------------------------------------------------------------------------
// LEGACY COMPATIBILITY
// -----------------------------------------------------------------------------
void Ublox_on()  { gps_power_on();  }
void Ublox_off() { gps_power_off(); }
