#include "Logging/logging_session.h"

#include <Arduino.h>

#include "Core/log.h"
#include "Core/Globals.h"
#include "Logging/logging_session_files.h"
#include "Storage/storage_manager.h"

namespace {
bool session_active = false;
}

bool logging_session_begin(const GpsFix& firstFix)
{
  if (session_active) return true;

  if (storage_is_shutting_down() || !Time_Set_OK) {
    LOG_STORAGE("Session", "begin rejected time=%d shutdown=%d",
                Time_Set_OK, storage_is_shutting_down());
    return false;
  }

  if (!logging_session_files_open()) {
    return false;
  }

  session_active = true;
  LOG_STORAGE("Session", "started sats=%u lat=%.6f lon=%.6f",
              firstFix.satellites, firstFix.lat, firstFix.lon);
  return true;
}

void logging_session_write_fix(const GpsFix& fix, bool writeLiveTrack)
{
  if (!session_active) return;

  logging_session_files_write_raw();
  (void)fix;
  (void)writeLiveTrack;
}

void logging_session_end()
{
  if (!session_active) return;

  LOG_STORAGE("Session", "ending");
  session_active = false;
  logging_session_files_close();
}

bool logging_session_active()
{
  return session_active;
}
