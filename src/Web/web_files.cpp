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
#include "Web/web_json.h"

namespace {
constexpr size_t FILE_LIST_JSON_BYTES = 16384;

// Return filename portion of a path. Callers must copy it before the owning
// File/String goes out of scope.
const char* basenameOnly(const char* path)
{
  if (!path) return nullptr;
  const char* p = strrchr(path, '/');
  return p ? p + 1 : path;
}

bool isAllowedLogExtension(const char* ext)
{
  return !strcasecmp(ext, ".txt") ||
         !strcasecmp(ext, ".sbp") ||
         !strcasecmp(ext, ".ubx") ||
         !strcasecmp(ext, ".geojson");
}

bool hasExtension(const char* name, const char* ext)
{
  const char* actual = strrchr(name, '.');
  return actual && !strcasecmp(actual, ext);
}

void buildLogPath(char* out, size_t outSize, const char* base)
{
  snprintf(out, outSize, "/logs/%s", base);
}

void buildPairedPath(char* out, size_t outSize, const char* base, const char* newExt)
{
  char stem[96];
  strlcpy(stem, base, sizeof(stem));

  char* dot = strrchr(stem, '.');
  if (dot) *dot = '\0';

  snprintf(out, outSize, "/logs/%s%s", stem, newExt);
}

// Validate log filename + extension, reject paths.
bool isValidLogFile(const char* name)
{
  if (!name || !*name) return false;
  if (strchr(name, '/') || strchr(name, '\\')) return false;

  const char* ext = strrchr(name, '.');
  return ext && isAllowedLogExtension(ext);
}

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

  if (hasExtension(base, ".geojson")) {
    buildPairedPath(pairedPath, sizeof(pairedPath), base, ".sbp");
  } else if (hasExtension(base, ".sbp")) {
    buildPairedPath(pairedPath, sizeof(pairedPath), base, ".geojson");
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
  buildPairedPath(sbpPath, sizeof(sbpPath), base, ".sbp");
  if (!storage.exists(sbpPath)) return;

  File sbp = storage.open(sbpPath, FILE_READ);
  if (!sbp) return;

  o["sbp_name"] = String(basenameOnly(sbpPath));
  o["sbp_size"] = sbp.size();
  sbp.close();
}

bool processLogDirectoryEntry(File& file, fs::FS& storage, JsonArray& files, int& removedEmpty)
{
  if (file.isDirectory()) {
    file.close();
    return false;
  }

  const char* base = basenameOnly(file.name());
  if (!isValidLogFile(base)) {
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
    buildLogPath(path, sizeof(path), baseCopy);
    if (storage.remove(path)) removedEmpty++;
    return false;
  }

  if (!hasExtension(baseCopy, ".geojson")) {
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
  if (!storage_on() || !server.hasArg("file")) {
    server.send(400);
    return;
  }

  String file = server.arg("file");

  const int q = file.indexOf('?');
  if (q >= 0) file = file.substring(0, q);

  const char* base = basenameOnly(file.c_str());
  if (!isValidLogFile(base)) {
    server.send(400);
    return;
  }

  char path[128];
  buildLogPath(path, sizeof(path), base);

  fs::FS& storage = storage_sd_fs();
  if (!storage.exists(path)) {
    server.send(404);
    return;
  }

  File f = storage.open(path, FILE_READ);
  if (!f) {
    server.send(500);
    return;
  }

  server.sendHeader("Content-Disposition", String("attachment; filename=\"") + base + "\"");
  server.sendHeader("Cache-Control", "no-store");

  const char* mime = strstr(base, ".geojson") ? "application/geo+json" : "application/octet-stream";
  server.streamFile(f, mime);
  f.close();
}

void handleFileDelete(WebServer& server)
{
  if (!storage_on() || !server.hasArg("plain")) {
    sendDeleteResult(server, false, 0);
    return;
  }

  StaticJsonDocument<256> j;
  if (deserializeJson(j, server.arg("plain"))) {
    server.send(400);
    return;
  }

  const char* base = basenameOnly(j["name"]);
  if (!isValidLogFile(base)) {
    server.send(400);
    return;
  }

  fs::FS& storage = storage_sd_fs();
  char path[128];
  buildLogPath(path, sizeof(path), base);

  const uint64_t fileBytes = fileSize(storage, path);
  const bool removed = storage.remove(path);
  const uint64_t pairedBytes = removed ? removePairedLogFile(storage, base) : 0;

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
