// ============================================================================
// logging_session.cpp
//
// High-level logging session controller. Enforces session state, delegates file
// lifecycle work, and accepts GPS fixes for raw log output while active.
// ============================================================================

#include "Logging/logging_session.h"

#include <Arduino.h>

#include "Core/log.h"
#include "Core/Globals.h"
#include "Logging/Session/logging_session_files.h"
#include "Storage/storage_manager.h"

namespace {
enum class SessionState {
  Idle,
  Active,
  Closing
};

SessionState session_state = SessionState::Idle;

bool isSessionActive()
{
  return session_state == SessionState::Active;
}

bool isSessionClosing()
{
  return session_state == SessionState::Closing;
}

bool sessionCanStart()
{
  if (isSessionActive()) return true;
  if (isSessionClosing()) {
    LOG_STORAGE("Session", "begin rejected while closing");
    return false;
  }

  if (storage_is_shutting_down() || !Time_Set_OK) {
    LOG_STORAGE("Session", "begin rejected time=%d shutdown=%d",
                Time_Set_OK, storage_is_shutting_down());
    return false;
  }

  return true;
}
}

bool logging_session_begin(const GpsFix& firstFix)
{
  if (!sessionCanStart()) return false;
  if (isSessionActive()) return true;

  if (!logging_session_files_open()) {
    return false;
  }

  session_state = SessionState::Active;
  LOG_STORAGE("Session", "started sats=%u lat=%.6f lon=%.6f",
              firstFix.satellites, firstFix.lat, firstFix.lon);
  return true;
}

void logging_session_write_fix()
{
  if (!isSessionActive()) return;

  logging_session_files_write_raw();
}

void logging_session_end()
{
  if (!isSessionActive()) return;

  LOG_STORAGE("Session", "ending");
  session_state = SessionState::Closing;
  const bool exported = logging_session_files_close();
  session_state = SessionState::Idle;
  LOG_STORAGE("Session", "ended export=%d", exported);
}

bool logging_session_active()
{
  return isSessionActive();
}
