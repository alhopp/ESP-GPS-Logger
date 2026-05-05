#include "Logging/logging_session.h"

#include <Arduino.h>

#include "Core/Definitions.h"
#include "Core/Globals.h"
#include "Logging/geojson.h"
#include "Logging/storage_file_operations.h"
#include "Storage/storage_manager.h"

namespace {
bool session_active = false;
uint32_t last_geojson_ms = 0;
}

bool logging_session_begin(const GpsFix& firstFix)
{
  if (session_active) return true;

  if (storage_is_shutting_down() || !Time_Set_OK) {
    LOG_STORAGE("Session", "begin rejected time=%d shutdown=%d",
                Time_Set_OK, storage_is_shutting_down());
    return false;
  }

  if (!storage_files_open()) {
    return false;
  }

  session_active = true;
  last_geojson_ms = 0;
  LOG_STORAGE("Session", "started sats=%u lat=%.6f lon=%.6f",
              firstFix.satellites, firstFix.lat, firstFix.lon);
  return true;
}

void logging_session_write_fix(const GpsFix& fix, bool writeLiveTrack)
{
  if (!session_active) return;

  storage_files_write_raw();

  if (!writeLiveTrack) return;

  const uint32_t now = millis();
  if (now - last_geojson_ms >= 1000) {
    last_geojson_ms = now;
    geojson_add_point(fix.lat, fix.lon);
  }
}

void logging_session_end()
{
  if (!session_active) return;

  LOG_STORAGE("Session", "ending");
  session_active = false;
  storage_files_close();
}

bool logging_session_active()
{
  return session_active;
}
