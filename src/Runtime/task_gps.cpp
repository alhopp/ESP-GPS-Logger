#include <Arduino.h>

#include "Core/Globals.h"
#include "GPS/Ublox/ublox_driver.h"
#include "Core/system_mode.h"
#include "Logging/logging_session.h"
#include "Runtime/gps_display_policy.h"
#include "Runtime/gps_logging_policy.h"
#include "Runtime/task_gps.h"

#include "GPS/gps_fix.h"
#include "GPS/gps_source.h"
#include "GPS/Metrics/gps_stats_service.h"

// -----------------------------------------------------------------------------
// GPS task state
// -----------------------------------------------------------------------------
namespace {
constexpr uint32_t IDLE_DELAY_MS = 200;
constexpr uint32_t POLL_DELAY_MS = 5;

bool gpsTaskShouldRun();
void processGpsFix(const GpsFix& fix);
void processGpsMessage(const GpsFix& fix);
void noteGpsSignalReady(const GpsFix& fix);
void maybeEnterLoggingMode();
void updateSessionStats(const GpsFix& fix);
}

// -----------------------------------------------------------------------------
// GPS task
// -----------------------------------------------------------------------------
void gpsTask(void *parameter)
{
  (void)parameter;

  for (;;) {
    if (!gpsTaskShouldRun()) {
      vTaskDelay(pdMS_TO_TICKS(IDLE_DELAY_MS));
      continue;
    }

    wdt_task0 = millis();

    if (gps_source_next_message() == MT_NAV_PVT) {
      processGpsFix(gps_fix_from_ubx());
    }

    vTaskDelay(pdMS_TO_TICKS(POLL_DELAY_MS));
  }
}

namespace {

bool gpsTaskShouldRun()
{
  const SystemMode mode = getMode();
  return mode == MODE_LOGGING || mode == MODE_WAIT_SATS;
}

void processGpsFix(const GpsFix& fix)
{
  processGpsMessage(fix);

  if (logging_session_active() && GPS_Signal_OK) {
    logging_session_write_fix(fix, getMode() == MODE_LOGGING);
  }

  gps_display_policy_update(fix);
}

void processGpsMessage(const GpsFix& fix)
{
  last_gps_msg = millis();

  if (logging_session_active()) nav_pvt_message++;

  noteGpsSignalReady(fix);
  maybeEnterLoggingMode();
  gps_logging_policy_maybe_start_session(fix);
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
    gps_logging_policy_note_signal_ready(millis());
  }
}

void maybeEnterLoggingMode()
{
  if (GPS_Signal_OK && getMode() == MODE_WAIT_SATS) {
    setMode(MODE_LOGGING);
  }
}

void updateSessionStats(const GpsFix& fix)
{
  if (!logging_session_active()) return;
  gps_stats_update(fix);
}

} // namespace
