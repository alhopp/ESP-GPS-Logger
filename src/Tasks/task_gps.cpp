#include <Arduino.h>

#include "Core/Globals.h"
#include "MANAGERS/config_manager.h"
#include "Ublox/ublox.h"
#include "core/system_mode.h"
#include "session/logging_session.h"
#include "tasks/task_display.h"
#include "tasks/task_gps.h"

#include "GPS/gps_alpha.h"
#include "GPS/gps_fix.h"
#include "GPS/gps_run.h"
#include "GPS/gps_source.h"
#include "GPS/gps_stats_service.h"

// -----------------------------------------------------------------------------
// GPS task state
// -----------------------------------------------------------------------------
int GPS_delay = 0;
TaskHandle_t t1 = nullptr;

namespace {
uint32_t timeWaitStartMs = 0;

void processGpsMessage(const GpsFix& fix);
void noteGpsSignalReady(const GpsFix& fix);
void maybeEnterLoggingMode();
void maybeStartLoggingSession(const GpsFix& fix);
void updateSessionStats(const GpsFix& fix);
void updateSatelliteWaitDisplay(const GpsFix& fix);
void updateSpeedDisplayThrottle(const GpsFix& fix);
}

// -----------------------------------------------------------------------------
// GPS task
// -----------------------------------------------------------------------------
void taskOne(void *parameter)
{
  for (;;) {
    if (getMode() != MODE_LOGGING && getMode() != MODE_WAIT_SATS) {
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    wdt_task0 = millis();

    if (gps_source_next_message() == MT_NAV_PVT) {
      const GpsFix fix = gps_fix_from_ubx();

      processGpsMessage(fix);

      if (logging_session_active() && GPS_Signal_OK) {
        logging_session_write_fix(fix, getMode() == MODE_LOGGING);
      }

      updateSatelliteWaitDisplay(fix);
      updateSpeedDisplayThrottle(fix);
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

namespace {

void processGpsMessage(const GpsFix& fix)
{
  last_gps_msg = millis();

  if (logging_session_active()) nav_pvt_message++;

  noteGpsSignalReady(fix);
  maybeEnterLoggingMode();
  maybeStartLoggingSession(fix);
  updateSessionStats(fix);
}

void noteGpsSignalReady(const GpsFix& fix)
{
  if (GPS_Signal_OK) return;

  if (fix.satellites >= MIN_numSV_FIRST_FIX &&
      fix.speedAccuracy < MAX_Sacc_FIRST_FIX &&
      fix.validDateTime) {
    GPS_Signal_OK = true;
    first_fix_GPS = millis() / 1000;
    timeWaitStartMs = millis();
  }
}

void maybeEnterLoggingMode()
{
  if (GPS_Signal_OK && getMode() == MODE_WAIT_SATS) {
    setMode(MODE_LOGGING);
  }
}

void maybeStartLoggingSession(const GpsFix& fix)
{
  if (!GPS_Signal_OK || Time_Set_OK || logging_session_active()) return;
  if (!fix.validDateTime && millis() - timeWaitStartMs <= 15000UL) return;

  if (fix.validDateTime) {
    Set_GPS_Time(config.timezone);
  }

  Time_Set_OK = true;
  Shut_down_Save_session = true;
  start_logging_millis = millis();

  reset_session_stats();
  logging_session_begin(fix);
}

void updateSessionStats(const GpsFix& fix)
{
  if (!logging_session_active()) return;
  gps_stats_update(fix);
}

void updateSatelliteWaitDisplay(const GpsFix& fix)
{
  static uint8_t lastSV = 0;

  if (getMode() != MODE_WAIT_SATS) return;

  if (fix.satellites != lastSV) {
    lastSV = fix.satellites;
    screen_request_partial(0, 100, 250, 122);
  }
}

void updateSpeedDisplayThrottle(const GpsFix& fix)
{
  static uint32_t lastSpeedUpdateMs = 0;

  if (getMode() != MODE_LOGGING || !GPS_Signal_OK) return;

  const float kts = fix.speedKnots;
  const uint32_t intervalMs =
    kts < 10.0f ? UINT32_MAX :
    kts < 20.0f ? 5000 :
    kts < 38.0f ? 3000 : 1000;

  const uint32_t now = millis();
  if (intervalMs != UINT32_MAX && now - lastSpeedUpdateMs >= intervalMs) {
    lastSpeedUpdateMs = now;
    screen_request_partial(0, 0, 250, 122);
  }
}

} // namespace
