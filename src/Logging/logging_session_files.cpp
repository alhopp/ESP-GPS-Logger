// ============================================================================
// logging_session_files.cpp
//
// Coordinates active session file handles. Opens UBX/SBP files, forwards raw
// sample writes, closes files, and finalizes GeoJSON export when a session ends.
// ============================================================================

#include <Arduino.h>
#include <FS.h>
#include <esp_system.h>

#include "Core/log.h"
#include "Core/Globals.h"
#include "Core/build_config.h"
#include "Logging/SBP/sbp_writer.h"
#include "Logging/GeoJSON/geojson_session_export.h"
#include "Logging/logging_raw_writers.h"
#include "Logging/logging_session_files.h"
#include "Logging/logging_session_paths.h"
#include "Logging/SBP/logging_sbp_debug.h"
#include "Session/session_stats_snapshot.h"
#include "Storage/storage_manager.h"
#include "Config/config_types.h"

namespace {
File ubxfile;
File sbpfile;
char activeSbpPath[128];
char activeGeoPath[128];

void closeFile(File& file)
{
  if (!file) return;

  file.flush();
  file.close();
  file = File();
}

void clearActiveSessionPaths()
{
  activeSbpPath[0] = '\0';
  activeGeoPath[0] = '\0';
}

bool hasActiveSessionPaths()
{
  return activeSbpPath[0] != '\0' && activeGeoPath[0] != '\0';
}

bool exportClosedSession()
{
  if (!hasActiveSessionPaths()) return false;

  const SessionStatsSnapshot snapshot = build_session_stats_snapshot();
  logging_sbp_debug_print_samples(activeSbpPath, snapshot);

  if (!geojson_session_export_finalize(activeSbpPath, activeGeoPath, snapshot)) {
    LOG_ERROR("STORAGE", "GeoJSON export failed");
    return false;
  }

  return true;
}
}

bool logging_session_files_open()
{
  if (storage_is_shutting_down() || !Time_Set_OK) {
    LOG_STORAGE("session_files", "called without valid GPS time");
    return false;
  }

  if (!storage_logs_dir_ready()) {
    LOG_ERROR("STORAGE", "logs dir unavailable");
    return false;
  }

  fs::FS& storage = storage_sd_fs();
  logging_raw_writers_reset();
  clearActiveSessionPaths();

  SessionPaths paths;
  logging_session_paths_build(paths);

  if (config.logUBX) {
    ubxfile = storage.open(paths.ubx, FILE_APPEND);
    if (!ubxfile) {
      LOG_ERROR("STORAGE", "UBX open failed");
      closeFile(ubxfile);
      return false;
    }
  }

  sbpfile = storage.open(paths.sbp, FILE_WRITE);
  if (!sbpfile) {
    LOG_ERROR("STORAGE", "SBP open failed");
    closeFile(ubxfile);
    closeFile(sbpfile);
    return false;
  }

  if (sbpfile.size() == 0) {
    sbp_write_header(sbpfile);
  }

  strlcpy(activeSbpPath, paths.sbp, sizeof(activeSbpPath));
  strlcpy(activeGeoPath, paths.geojson, sizeof(activeGeoPath));

  LOG_STORAGE("LOG", "Session started %s", paths.base);
  return true;
}

void logging_session_files_write_raw()
{
  if (storage_is_shutting_down() || !Time_Set_OK) return;

  logging_raw_writers_write_ubx(ubxfile);
  logging_raw_writers_write_sbp(sbpfile);
}

bool logging_session_files_close()
{
  closeFile(sbpfile);
  const bool exported = exportClosedSession();
  closeFile(ubxfile);
  closeFile(sbpfile);
  clearActiveSessionPaths();
  return exported;
}
