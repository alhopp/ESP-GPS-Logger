// -----------------------------------------------------------------------------
// system_mode.cpp
//
// Central system mode state machine.
//
// This module owns the authoritative SystemMode and performs side-effect
// transitions between modes: Wi-Fi, GPS power, storage, and session shutdown.
// -----------------------------------------------------------------------------

#include "Core/system_mode.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Core/log.h"
#include "Core/Rtc/rtc_session_stats.h"
#include "GPS/Data/gps_runtime_instances.h"
#include "GPS/Hardware/gps_manager.h"
#include "Storage/storage_manager.h"
#include "Logging/logging_session.h"
#include "Runtime/display_redraw.h"
#include "Web/web_server.h"
#include "Web/wifi_manager.h"

namespace {
constexpr uint32_t IDLE_AUTO_SLEEP_MS = 30000;
constexpr uint32_t CONFIG_AUTO_SLEEP_MS = 10UL * 60UL * 1000UL;

volatile SystemMode currentMode = MODE_BOOT;
volatile bool enterModeFailed = false;
uint32_t modeEnteredAtMs = 0;

void requestModeRedraw()
{
  screen_request_partial(DISPLAY_FULL_WINDOW);
}

void beginStorageShutdown()
{
  storage_begin_shutdown();
  screen_request_redraw();
  vTaskDelay(pdMS_TO_TICKS(20));
}

void ensureStorageReady(const char* context)
{
  if (!storage_on()) {
    LOG_ERROR("SD", "storage_on failed in %s", context);
  }
}

bool modeNeedsStorage(SystemMode mode)
{
  return mode == MODE_WAIT_SATS ||
         mode == MODE_LOGGING ||
         mode == MODE_CONFIG;
}

void stopLoggingStorage()
{
  logging_session_end();
  storage_off();
}

void exitLogging()
{
  alpha_500m.Finalise_Run();
#if STATS_ONLY_SERIAL
  alpha_500m_60.Finalise_Run();
  alpha_500m_70.Finalise_Run();
#endif
  rtc_snapshot_stats();

  beginStorageShutdown();
  stopLoggingStorage();
}

void exitConfig()
{
  LOG_SYS("MODE", "EXIT CONFIG");

  webserver_stop();
  wifi_stop();
  storage_off();
}

void exitSleep()
{
  LOG_SYS("MODE", "EXIT SLEEP");
}

void exitWaitSats(SystemMode newMode)
{
  if (newMode != MODE_LOGGING) {
    gps_shutdown();
    storage_off();
  }
}

void runExitActions(SystemMode oldMode, SystemMode newMode)
{
  switch (oldMode) {
    case MODE_LOGGING:
      exitLogging();
      break;

    case MODE_WAIT_SATS:
      exitWaitSats(newMode);
      break;

    case MODE_CONFIG:
      exitConfig();
      break;

    case MODE_SLEEP:
      exitSleep();
      break;

    case MODE_BOOT:
    case MODE_IDLE:
    case MODE_ERROR:
    default:
      break;
  }
}

void enterLogging()
{
  LOG_SYS("MODE", "ENTER LOGGING, GPS already active, Wi-Fi OFF");

  storage_end_shutdown();

  wifi_stop();

  ensureStorageReady("LOGGING");
}

void enterWaitSats()
{
  LOG_SYS("MODE", "ENTER WAIT_SATS, GPS ON");

  storage_end_shutdown();
  wifi_stop();
  ensureStorageReady("WAIT_SATS");

  if (!initGPS()) {
    LOG_ERROR("GPS", "init failed entering WAIT_SATS");
    storage_off();
    enterModeFailed = true;
  }
}

void enterConfig()
{
  storage_end_shutdown();

  LOG_SYS("MODE", "ENTER CONFIG");

  gps_shutdown();

  ensureStorageReady("CONFIG");

  wifi_init();

  if (wifi_net_active()) {
    LOG_SYS("MODE", "CONFIG online (network active)");
    webserver_start();
  } else {
    LOG_SYS("MODE", "CONFIG offline (no network)");
  }
}

void enterSleep()
{
  LOG_SYS("MODE", "ENTER SLEEP");

  storage_begin_shutdown();
  storage_off();
  wifi_stop();
  gps_shutdown();
}

void runEnterActions(SystemMode newMode)
{
  switch (newMode) {
    case MODE_WAIT_SATS:
      enterWaitSats();
      break;

    case MODE_LOGGING:
      enterLogging();
      break;

    case MODE_CONFIG:
      enterConfig();
      break;

    case MODE_SLEEP:
      enterSleep();
      break;

    case MODE_BOOT:
    case MODE_IDLE:
    case MODE_ERROR:
    default:
      break;
  }
}

void recoverFromFailedEnter(SystemMode requestedMode)
{
  if (!enterModeFailed || currentMode != requestedMode) return;

  currentMode = MODE_IDLE;
  modeEnteredAtMs = millis();
  requestModeRedraw();
}
}

SystemMode getMode()
{
  return currentMode;
}

const char* modeToString(SystemMode mode)
{
  switch (mode) {
    case MODE_BOOT:      return "BOOT";
    case MODE_IDLE:      return "IDLE";
    case MODE_WAIT_SATS: return "WAIT_SATS";
    case MODE_CONFIG:    return "CONFIG";
    case MODE_LOGGING:   return "LOGGING";
    case MODE_SLEEP:     return "SLEEP";
    case MODE_ERROR:     return "ERROR";
    default:             return "?";
  }
}

void systemModeLoop()
{
  const SystemMode mode = getMode();

  if (mode == MODE_IDLE) {
    if (millis() - modeEnteredAtMs >= IDLE_AUTO_SLEEP_MS) {
      setMode(MODE_SLEEP);
    }
    return;
  }

  if (mode != MODE_CONFIG) return;

  wifi_loop();

  if (wifi_net_active()) {
    webserver_start();
    webserver_loop();
  }

  const uint32_t lastActivity = webserver_last_activity_ms();
  const uint32_t activityBase = lastActivity ? lastActivity : modeEnteredAtMs;
  if (millis() - activityBase >= CONFIG_AUTO_SLEEP_MS) {
    setMode(MODE_SLEEP);
  }
}

void setMode(SystemMode newMode)
{
  LOG_SYS("MODE", "REQUEST %s", modeToString(newMode));

  if (newMode == currentMode) {
    return;
  }

  const SystemMode oldMode = currentMode;

  LOG_SYS("MODE", "EXIT %s -> ENTER %s",
          modeToString(oldMode),
          modeToString(newMode));

  enterModeFailed = false;

  if (oldMode == MODE_LOGGING) {
    currentMode = newMode;
  }

  runExitActions(oldMode, newMode);

  if (modeNeedsStorage(newMode)) {
    storage_end_shutdown();
  }

  currentMode = newMode;
  modeEnteredAtMs = millis();

  runEnterActions(newMode);
  requestModeRedraw();

  recoverFromFailedEnter(newMode);
}
