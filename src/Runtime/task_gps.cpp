#include <Arduino.h>

// ============================================================================
// GPS runtime task
//
// Runs only while the system is waiting for satellites or actively logging.
// Each NAV-PVT fix flows through the logging policy, session writer, metrics
// update, and display policy in that order.
// ============================================================================

#include "Core/build_config.h"
#include "Core/Globals.h"
#include "GPS/gps_runtime_state.h"
#include "GPS/Ublox/ublox_driver.h"
#include "Core/system_mode.h"
#include "Logging/logging_session.h"
#include "Runtime/gps_display_policy.h"
#include "Runtime/gps_logging_policy.h"
#include "Runtime/task_gps.h"

#include "GPS/gps_fix.h"
#include "GPS/gps_config.h"
#include "GPS/Source/gps_source.h"
#include "GPS/Metrics/gps_stats_service.h"
#include "Storage/storage_manager.h"

// -----------------------------------------------------------------------------
// GPS task state
// -----------------------------------------------------------------------------
namespace {
constexpr uint32_t IDLE_DELAY_MS = 200;
constexpr uint32_t POLL_DELAY_MS = 5;
#if GPS_SIMULATOR
constexpr int GPS_SIM_BURST_LIMIT = 16;
#endif

bool gpsTaskShouldRun();
void processGpsFix(const GpsFix& fix);
void processGpsMessage(const GpsFix& fix);
void noteGpsSignalReady(const GpsFix& fix);
void maybeEnterLoggingMode(bool sessionActive);
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

#if GPS_SIMULATOR
    for (int i = 0; i < GPS_SIM_BURST_LIMIT; i++) {
      if (gps_source_next_message() != MT_NAV_PVT) break;
      processGpsFix(gps_fix_from_ubx());
    }
#else
    if (gps_source_next_message() == MT_NAV_PVT) {
      processGpsFix(gps_fix_from_ubx());
    }
#endif

    vTaskDelay(pdMS_TO_TICKS(POLL_DELAY_MS));
  }
}

namespace {

bool gpsTaskShouldRun()
{
  if (storage_is_shutting_down()) return false;

  const SystemMode mode = getMode();
  return mode == MODE_LOGGING || mode == MODE_WAIT_SATS;
}

void processGpsFix(const GpsFix& fix)
{
  if (logging_session_active()) nav_pvt_message++;

  processGpsMessage(fix);

  if (logging_session_active() && GPS_Signal_OK) {
    logging_session_write_fix();
  }

  gps_display_policy_update(fix);
}

void processGpsMessage(const GpsFix& fix)
{
  last_gps_msg = millis();

  noteGpsSignalReady(fix);
  const bool sessionActive = gps_logging_policy_maybe_start_session(fix);
  maybeEnterLoggingMode(sessionActive);
  updateSessionStats(fix);
}

void noteGpsSignalReady(const GpsFix& fix)
{
  if (GPS_Signal_OK) return;

  if (fix.satellites >= MIN_numSV_FIRST_FIX &&
      fix.speedAccuracy < MAX_Sacc_FIRST_FIX &&
      fix.validDateTime) {
    GPS_Signal_OK = true;
    gps_logging_policy_note_signal_ready(millis());
  }
}

void maybeEnterLoggingMode(bool sessionActive)
{
  if (sessionActive && getMode() == MODE_WAIT_SATS) {
    setMode(MODE_LOGGING);
  }
}

void updateSessionStats(const GpsFix& fix)
{
  if (!logging_session_active()) return;
  gps_stats_update(fix);
}

} // namespace
