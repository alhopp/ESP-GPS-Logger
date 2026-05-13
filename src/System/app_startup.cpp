#include "System/app_startup.h"

#include "Config/config_manager.h"
#include "Core/Battery/battery_monitor.h"
#include "Core/Globals.h"
#include "Core/log.h"
#include "Core/magnet_input.h"
#include "Core/sleep_control.h"
#include "Core/system_mode.h"
#include "Runtime/task_runtime.h"
#include "Storage/storage_manager.h"
#include "System/boot_manager.h"

namespace {

void initSubsystems()
{
  initStorage();
  initConfig();
  battery_sample();
  initMagnet();
}

bool startTasksOrEnterError()
{
  if (startRuntimeTasks()) {
    return true;
  }

  setMode(MODE_ERROR);
  return false;
}

}

void appStartup()
{
  LOG_SYS("Setup", "start");

  setMode(MODE_BOOT);

  woke_from_sleep = sleep_woke_from_magnet();

  if (initBoot() != BOOT_OK) {
    setMode(MODE_SLEEP);
    return;
  }

  initSubsystems();

  if (!startTasksOrEnterError()) {
    return;
  }

  setMode(MODE_IDLE);
  LOG_SYS("Setup", "done");
}
