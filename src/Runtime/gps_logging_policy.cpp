#include "Runtime/gps_logging_policy.h"

#include <Arduino.h>

#include "Config/config_types.h"
#include "Core/Globals.h"
#include "GPS/Data/gps_data.h"
#include "GPS/Ublox/ublox_driver.h"
#include "Logging/logging_session.h"

namespace {
constexpr uint32_t TIME_SYNC_WAIT_MS = 15000UL;

uint32_t timeWaitStartMs = 0;
}

void gps_logging_policy_note_signal_ready(uint32_t nowMs)
{
  timeWaitStartMs = nowMs;
}

void gps_logging_policy_maybe_start_session(const GpsFix& fix)
{
  if (!GPS_Signal_OK || logging_session_active()) return;

  if (!Time_Set_OK) {
    if (!fix.validDateTime && millis() - timeWaitStartMs <= TIME_SYNC_WAIT_MS) return;

    if (fix.validDateTime) {
      Set_GPS_Time(config.timezone);
    }

    Time_Set_OK = true;
  }

  if (!logging_session_begin(fix)) return;

  Shut_down_Save_session = true;
  start_logging_millis = millis();

  reset_session_stats();
}
