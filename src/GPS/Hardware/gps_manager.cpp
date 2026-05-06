// ============================================================================
// gps_manager.cpp
//
// GPS startup orchestration for ESP32 + u-blox.
// Owns power-on, baud probing, and receiver configuration. Runtime sample
// parsing continues through gps_source_next_message().
// ============================================================================

#include "GPS/Hardware/gps_manager.h"

#include <Arduino.h>

#include "Core/board_pins.h"
#include "Core/build_config.h"
#include "Core/log.h"
#include "GPS/Ublox/ublox_driver.h"
#include "GPS/Hardware/gps_power.h"
#include "GPS/Hardware/gps_startup_time.h"

tm tmstruct{};
int Time_Set_OK = 0;

namespace {
GpsLifecycleState lifecycleState = GpsLifecycleState::Off;

void setLifecycleState(GpsLifecycleState state)
{
  lifecycleState = state;
}

bool probeGps(uint32_t baud)
{
  // Try one baud rate at a time. Some receivers may boot at their configured
  // rate, while factory/default units can appear at 9600.
  UbloxSerial.end();
  delay(20);
  UbloxSerial.begin(baud, SERIAL_8N1, GPS_UART_RX_PIN, GPS_UART_TX_PIN);
  delay(120);

  // Drop any stale bytes before asking for MON-VER.
  while (UbloxSerial.available()) {
    UbloxSerial.read();
  }

  sendUbx(ubx::poll::mon_ver);

  const uint32_t start = millis();
  while (millis() - start < 1000) {
    if (processGPS() == MT_MON_VER &&
        (ubxMessage.monVER.hwVersion[3] == '8' ||
         ubxMessage.monVER.hwVersion[3] == '9' ||
         ubxMessage.monVER.hwVersion[3] == 'A')) {
      LOG_GPS("Probe", "GPS responded @%lu baud hw=%s",
              (unsigned long)baud, ubxMessage.monVER.hwVersion);
      return true;
    }
    delay(1);
  }

  return false;
}
}

bool initGPS()
{
#if GPS_SIMULATOR
  LOG_GPS("Init", "simulator enabled, skipping hardware init");
  setLifecycleState(GpsLifecycleState::Ready);
  return true;
#else
  LOG_GPS("Init", "starting");
  setLifecycleState(GpsLifecycleState::Starting);

  // Power is managed once here. Runtime code should read messages only; it
  // should not attempt a second bring-up.
  gps_power_on();
  delay(100);

  if (!probeGps(38400) &&
      !probeGps(9600) &&
      !probeGps(115200)) {
    LOG_GPS("Init", "no GPS response");
    gps_power_off();
    setLifecycleState(GpsLifecycleState::Failed);
    return false;
  }

  Init_ubloxM10();
  gps_send_time_from_rtc();

  LOG_GPS("Init", "GPS ready");
  setLifecycleState(GpsLifecycleState::Ready);
  return true;
#endif
}

void gps_shutdown()
{
#if GPS_SIMULATOR
  setLifecycleState(GpsLifecycleState::Off);
#else
  gps_power_off();
  setLifecycleState(GpsLifecycleState::Off);
#endif
}

const char* gps_lifecycle_state_name()
{
  switch (lifecycleState) {
    case GpsLifecycleState::Off:      return "Off";
    case GpsLifecycleState::Starting: return "Starting";
    case GpsLifecycleState::Ready:    return "Ready";
    case GpsLifecycleState::Failed:   return "Failed";
    default:                          return "?";
  }
}
