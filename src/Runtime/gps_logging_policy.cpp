#include "Runtime/gps_logging_policy.h"

#include <Arduino.h>

#include "Config/config_types.h"
#include "Core/Globals.h"
#include "GPS/Data/gps_session_reset.h"
#include "GPS/gps_runtime_state.h"
#include "GPS/Ublox/ublox_driver.h"
#include "Logging/logging_session.h"

namespace {
constexpr uint32_t TIME_SYNC_WAIT_MS = 15000UL;
constexpr uint32_t SESSION_BEGIN_RETRY_MS = 250UL;

uint32_t timeWaitStartMs = 0;
uint32_t lastSessionBeginAttemptMs = 0;

bool ensureGpsTimeReady(const GpsFix& fix)
{
  if (Time_Set_OK) return true;

  if (!fix.validDateTime && millis() - timeWaitStartMs <= TIME_SYNC_WAIT_MS) return false;

  if (fix.validDateTime) {
    Set_GPS_Time(config.timezone);
  }

  Time_Set_OK = true;
  return true;
}

bool sessionBeginRetryDue(uint32_t nowMs)
{
  if (nowMs - lastSessionBeginAttemptMs < SESSION_BEGIN_RETRY_MS) return false;
  lastSessionBeginAttemptMs = nowMs;
  return true;
}
}

void gps_logging_policy_note_signal_ready(uint32_t nowMs)
{
  timeWaitStartMs = nowMs;
}

bool gps_logging_policy_maybe_start_session(const GpsFix& fix)
{
  if (!GPS_Signal_OK) return false;
  if (logging_session_active()) return true;

  if (!ensureGpsTimeReady(fix)) return false;

  const uint32_t now = millis();
  if (!sessionBeginRetryDue(now)) return false;

  if (!logging_session_begin(fix)) return false;

  reset_session_stats();
  return true;
}
