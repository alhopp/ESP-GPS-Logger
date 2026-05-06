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
#include "GPS/Hardware/gps_manager.h"
#include "GPS/Metrics/gps_alpha_speed.h"
#include "Storage/storage_manager.h"
#include "Logging/logging_session.h"
#include "Runtime/display_redraw.h"
#include "Web/web_server.h"
#include "Web/wifi_manager.h"

namespace {
volatile SystemMode currentMode = MODE_BOOT;
volatile bool enterModeFailed = false;

void requestModeRedraw()
{
  screen_request_partial(DISPLAY_FULL_WINDOW);
}

void beginStorageShutdown()
{
  storage_begin_shutdown();
  vTaskDelay(pdMS_TO_TICKS(20));
}

void ensureStorageReady(const char* context)
{
  if (!storage_on()) {
    LOG_ERROR("SD", "storage_on failed in %s", context);
  }
}

void stopLoggingStorage()
{
  logging_session_end();
  storage_off();
}

void exitLogging()
{
  A500.Finalise_Run();
  rtc_snapshot_stats();

  beginStorageShutdown();
  stopLoggingStorage();
}

void exitConfig()
{
  LOG_SYS("MODE", "EXIT CONFIG");

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
  if (getMode() != MODE_CONFIG) return;

  wifi_loop();

  if (wifi_net_active()) {
    webserver_loop();
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

  runExitActions(oldMode, newMode);

  currentMode = newMode;
  requestModeRedraw();

  runEnterActions(newMode);

  if (enterModeFailed && currentMode == newMode) {
    currentMode = MODE_IDLE;
    requestModeRedraw();
  }
}
