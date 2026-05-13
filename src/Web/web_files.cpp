// ============================================================================
// web_files.cpp
//
// Web API for SD log file access (CONFIG mode only).
// Provides endpoints to list, download, and delete log files under /logs.
// Safety: strict filename validation, directory confinement, streaming only.
// Notes: File.name() buffers are transient; large JSON uses heap.
// ============================================================================

#include "web_files.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>

#include "Core/log.h"
#include "Storage/storage_manager.h"
#include "Web/web_file_paths.h"
#include "Web/web_json.h"
#include "Web/web_server.h"

namespace {
constexpr size_t FILE_LIST_JSON_BYTES = 16384;

uint64_t fileSize(fs::FS& storage, const char* path)
{
  File file = storage.open(path, FILE_READ);
  if (!file) return 0;

  const uint64_t size = file.size();
  file.close();
  return size;
}

uint64_t removePairedLogFile(fs::FS& storage, const char* base)
{
  char pairedPath[128];

  if (web_log_has_extension(base, ".geojson")) {
    web_log_build_paired_path(pairedPath, sizeof(pairedPath), base, ".sbp");
  } else if (web_log_has_extension(base, ".sbp")) {
    web_log_build_paired_path(pairedPath, sizeof(pairedPath), base, ".geojson");
  } else {
    return 0;
  }

  if (!storage.exists(pairedPath)) return 0;

  const uint64_t size = fileSize(storage, pairedPath);
  return storage.remove(pairedPath) ? size : 0;
}

void sendDeleteResult(WebServer& server, bool ok, uint64_t deletedBytes)
{
  StaticJsonDocument<128> response;
  response["ok"] = ok;
  response["deleted_bytes"] = static_cast<double>(deletedBytes);
  web_send_json(server, response);
}

void addStorageStats(JsonDocument& j)
{
  j["storage_used_mb"] = storage_sd_used_mb();
  j["storage_free_mb"] = storage_sd_free_mb();
  j["storage_used_bytes"] = storage_sd_used_bytes();
  j["storage_free_bytes"] = storage_sd_free_bytes();
}

void addEmptyStorageStats(JsonDocument& j)
{
  j["storage_used_mb"] = 0;
  j["storage_free_mb"] = 0;
  j["storage_used_bytes"] = 0;
  j["storage_free_bytes"] = 0;
}

void addGeojsonFile(JsonArray& files, fs::FS& storage, const char* base, size_t size, time_t modified)
{
  JsonObject o = files.createNestedObject();
  o["name"] = String(base);
  o["size"] = size;
  o["mtime"] = static_cast<uint32_t>(modified);

  char sbpPath[128];
  web_log_build_paired_path(sbpPath, sizeof(sbpPath), base, ".sbp");
  if (!storage.exists(sbpPath)) return;

  File sbp = storage.open(sbpPath, FILE_READ);
  if (!sbp) return;

  o["sbp_name"] = String(web_log_basename(sbpPath));
  o["sbp_size"] = sbp.size();
  sbp.close();
}

bool processLogDirectoryEntry(File& file, fs::FS& storage, JsonArray& files, int& removedEmpty)
{
  if (file.isDirectory()) {
    file.close();
    return false;
  }

  const char* base = web_log_basename(file.name());
  if (!web_log_is_valid_file(base)) {
    file.close();
    return false;
  }

  char baseCopy[96];
  strlcpy(baseCopy, base, sizeof(baseCopy));

  const size_t size = file.size();
  const time_t modified = file.getLastWrite();
  file.close();

  if (size == 0) {
    char path[128];
    web_log_build_path(path, sizeof(path), baseCopy);
    if (storage.remove(path)) removedEmpty++;
    return false;
  }

  if (!web_log_has_extension(baseCopy, ".geojson")) {
    return false;
  }

  addGeojsonFile(files, storage, baseCopy, size, modified);
  return true;
}

void handleFilesList(WebServer& server)
{
  LOG_STORAGE("API files", "request");

  DynamicJsonDocument j(FILE_LIST_JSON_BYTES);
  j["ok"] = true;

  if (!storage_logs_dir_ready()) {
    LOG_STORAGE("API files", "storage not ready");
    j["ok"] = false;
    addEmptyStorageStats(j);
    web_send_json(server, j);
    return;
  }

  addStorageStats(j);
  JsonArray files = j.createNestedArray("files");

  fs::FS& storage = storage_sd_fs();
  File dir = storage.open("/logs");
  if (!dir || !dir.isDirectory()) {
    LOG_STORAGE("API files", "/logs unavailable");
    j["ok"] = false;
    web_send_json(server, j);
    return;
  }

  int count = 0;
  int removedEmpty = 0;
  File file = dir.openNextFile();
  while (file) {
    if (processLogDirectoryEntry(file, storage, files, removedEmpty)) {
      count++;
    }
    file = dir.openNextFile();
  }
  dir.close();

  if (j.overflowed()) {
    LOG_ERROR("API files", "JSON overflow after %d files", count);
    j.clear();
    j["ok"] = false;
    j["error"] = "too_many_files";
    web_send_json(server, j);
    return;
  }

  LOG_STORAGE("API files", "%d sessions, removed %d empty", count, removedEmpty);
  web_send_json(server, j);
}

void handleFileDownload(WebServer& server)
{
  webserver_note_activity();

  if (!storage_on() || !server.hasArg("file")) {
    server.send(400);
    return;
  }

  String file = server.arg("file");

  const int q = file.indexOf('?');
  if (q >= 0) file = file.substring(0, q);

  ValidatedLogPath logFile;
  if (!web_log_resolve_path(file.c_str(), logFile)) {
    server.send(400);
    return;
  }

  fs::FS& storage = storage_sd_fs();
  if (!storage.exists(logFile.path)) {
    server.send(404);
    return;
  }

  File f = storage.open(logFile.path, FILE_READ);
  if (!f) {
    server.send(500);
    return;
  }

  server.sendHeader("Content-Disposition", String("attachment; filename=\"") + logFile.base + "\"");
  server.sendHeader("Cache-Control", "no-store");

  const char* mime = strstr(logFile.base, ".geojson") ? "application/geo+json" : "application/octet-stream";
  server.streamFile(f, mime);
  f.close();
}

void handleFileDelete(WebServer& server)
{
  webserver_note_activity();

  if (!storage_on() || !server.hasArg("plain")) {
    sendDeleteResult(server, false, 0);
    return;
  }

  StaticJsonDocument<256> j;
  if (deserializeJson(j, server.arg("plain"))) {
    server.send(400);
    return;
  }

  ValidatedLogPath logFile;
  if (!web_log_resolve_path(j["name"], logFile)) {
    server.send(400);
    return;
  }

  fs::FS& storage = storage_sd_fs();
  const uint64_t fileBytes = fileSize(storage, logFile.path);
  const bool removed = storage.remove(logFile.path);
  const uint64_t pairedBytes = removed ? removePairedLogFile(storage, logFile.base) : 0;

  sendDeleteResult(server, removed, removed ? fileBytes + pairedBytes : 0);
}
} // namespace

// -----------------------------------------------------------------------------
// Endpoint registration
// -----------------------------------------------------------------------------

void registerFileEndpoints(WebServer &server)
{
  server.on("/api/files", HTTP_GET, [&] { handleFilesList(server); });
  server.on("/api/download", HTTP_GET, [&] { handleFileDownload(server); });
  server.on("/api/file", HTTP_DELETE, [&] { handleFileDelete(server); });
}
