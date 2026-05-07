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

void sendJsonOk(WebServer& server, bool ok)
{
  server.send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

void removePairedLogFile(fs::FS& storage, const char* base)
{
  char pairedPath[128];

  if (hasExtension(base, ".geojson")) {
    buildPairedPath(pairedPath, sizeof(pairedPath), base, ".sbp");
  } else if (hasExtension(base, ".sbp")) {
    buildPairedPath(pairedPath, sizeof(pairedPath), base, ".geojson");
  } else {
    return;
  }

  if (storage.exists(pairedPath)) {
    storage.remove(pairedPath);
  }
}
} // namespace

// -----------------------------------------------------------------------------
// Endpoint registration
// -----------------------------------------------------------------------------

void registerFileEndpoints(WebServer &server)
{
  // ---------------------------------------------------------------------------
  // GET /api/files
  // List log files in /logs (name + size only)
  // ---------------------------------------------------------------------------
  server.on("/api/files", HTTP_GET, [&] {
    LOG_STORAGE("API files", "request");

    DynamicJsonDocument j(FILE_LIST_JSON_BYTES);
    j["ok"] = true;
    JsonArray files = j.createNestedArray("files");

    if (!storage_logs_dir_ready()) {
      LOG_STORAGE("API files", "storage not ready");
      j["ok"] = false;
      web_send_json(server, j);
      return;
    }

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
      if (!file.isDirectory()) {
        const char* base = basenameOnly(file.name());
        if (isValidLogFile(base)) {
          char baseCopy[96];
          strlcpy(baseCopy, base, sizeof(baseCopy));

          const size_t size = file.size();
          const time_t modified = file.getLastWrite();
          file.close();

          if (size == 0) {
            char path[128];
            buildLogPath(path, sizeof(path), baseCopy);
            if (storage.remove(path)) removedEmpty++;
          } else if (hasExtension(baseCopy, ".geojson")) {
            JsonObject o = files.createNestedObject();
            o["name"] = String(baseCopy);
            o["size"] = size;
            o["mtime"] = static_cast<uint32_t>(modified);

            char sbpPath[128];
            buildPairedPath(sbpPath, sizeof(sbpPath), baseCopy, ".sbp");
            if (storage.exists(sbpPath)) {
              File sbp = storage.open(sbpPath, FILE_READ);
              if (sbp) {
                o["sbp_name"] = String(basenameOnly(sbpPath));
                o["sbp_size"] = sbp.size();
                sbp.close();
              }
            }

            count++;
          }
        } else {
          file.close();
        }
      } else {
        file.close();
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
  });

  // ---------------------------------------------------------------------------
  // GET /api/download?file=...
  // Stream validated log file (forced download)
  // ---------------------------------------------------------------------------
  server.on("/api/download", HTTP_GET, [&] {

    if (!storage_on() || !server.hasArg("file")) { server.send(400); return; }

    String file = server.arg("file");

    // Strip cache-buster (?t=...)
    int q = file.indexOf('?');
    if (q >= 0) file = file.substring(0, q);

    const char* base = basenameOnly(file.c_str());
    if (!isValidLogFile(base)) { server.send(400); return; }

    char path[128];
    buildLogPath(path, sizeof(path), base);

    fs::FS& storage = storage_sd_fs();
    if (!storage.exists(path)) { server.send(404); return; }

    File f = storage.open(path, FILE_READ);
    if (!f) { server.send(500); return; }

    // ---- Force download (instead of inline open) ----
    server.sendHeader("Content-Disposition", String("attachment; filename=\"") + base + "\"");
    server.sendHeader("Cache-Control", "no-store");

    const char* mime = "application/octet-stream";
    if (strstr(base, ".geojson")) mime = "application/geo+json";

    server.streamFile(f, mime);
    f.close();
  });

  // ---------------------------------------------------------------------------
  // DELETE /api/file
  // Delete a single validated log file
  // ---------------------------------------------------------------------------
  server.on("/api/file", HTTP_DELETE, [&] {

    if (!storage_on() || !server.hasArg("plain")) {
      sendJsonOk(server, false);
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

    const bool removed = storage.remove(path);
    if (removed) removePairedLogFile(storage, base);

    sendJsonOk(server, removed);
  });
}
