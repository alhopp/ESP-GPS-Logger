// ============================================================================
// gps_manager.cpp
//
// GPS startup orchestration for ESP32 + u-blox.
// ============================================================================

#include "GPS/gps_manager.h"

#include <Arduino.h>

#include "Core/board_pins.h"
#include "Core/Definitions.h"
#include "GPS/Ublox/ublox_driver.h"
#include "GPS/gps_power.h"
#include "GPS/gps_startup_time.h"

tm tmstruct{};
int Time_Set_OK = 0;

namespace {
bool probeGps(uint32_t baud)
{
  UbloxSerial.begin(baud, SERIAL_8N1, GPS_UART_RX_PIN, GPS_UART_TX_PIN);
  delay(120);

  // Send MON-VER poll. Do not drain RX here; processGPS() consumes bytes later.
  UbloxSerial.write(ubx::poll::mon_ver, sizeof(ubx::poll::mon_ver));
  UbloxSerial.flush();

  const uint32_t start = millis();
  while (millis() - start < 300) {
    if (UbloxSerial.available() > 0) {
      return true;
    }
    delay(1);
  }

  return false;
}
}

bool initGPS()
{
  LOG_GPS("Init", "starting");

  gps_power_on();
  delay(100);

  UbloxSerial.end();
  delay(20);
  UbloxSerial.begin(38400, SERIAL_8N1, GPS_UART_RX_PIN, GPS_UART_TX_PIN);
  delay(100);

  if (!probeGps(38400)) {
    LOG_GPS("Init", "no GPS response");
    gps_power_off();
    return false;
  }

  Init_ubloxM10();
  gps_send_time_from_rtc();

  LOG_GPS("Init", "GPS ready");
  return true;
}
