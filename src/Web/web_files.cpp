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

#include "Storage/storage_manager.h"

// -----------------------------------------------------------------------------
// Helpers (local)
// -----------------------------------------------------------------------------

// Return filename portion of a path (caller must copy if needed)
static const char* basenameOnly(const char* path)
{
  if (!path) return nullptr;
  const char* p = strrchr(path, '/');
  return p ? p + 1 : path;
}

// Validate log filename + extension, reject paths
static bool isValidLogFile(const char* name)
{
  if (!name || !*name) return false;
  if (strchr(name, '/') || strchr(name, '\\')) return false;

  const char* ext = strrchr(name, '.');
  if (!ext) return false;

  return !strcasecmp(ext, ".txt")  ||
         !strcasecmp(ext, ".sbp")  ||
         !strcasecmp(ext, ".ubx")  ||
         !strcasecmp(ext, ".geojson");
}
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

    DynamicJsonDocument j(8192);
    j["ok"] = true;
    JsonArray files = j.createNestedArray("files");

    fs::FS& storage = storage_sd_fs();
    File dir = storage.open("/logs");
    if (!dir || !dir.isDirectory()) {
      j["ok"] = false;
      server.send(200, "application/json", j.as<String>());
      return;
    }

    File file = dir.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        const char* base = basenameOnly(file.name());
        if (isValidLogFile(base)) {
          JsonObject o = files.createNestedObject();
          o["name"] = base;
          o["size"] = file.size();
        }
      }

      file = dir.openNextFile();
    }

    server.send(200, "application/json", j.as<String>());
  });

  // ---------------------------------------------------------------------------
  // GET /api/download?file=...
  // Stream validated log file (forced download)
  // ---------------------------------------------------------------------------
  server.on("/api/download", HTTP_GET, [&] {

    if (!storage_sd_available() || !server.hasArg("file")) { server.send(400); return; }

    String file = server.arg("file");

    // Strip cache-buster (?t=...)
    int q = file.indexOf('?');
    if (q >= 0) file = file.substring(0, q);

    const char* base = basenameOnly(file.c_str());
    if (!isValidLogFile(base)) { server.send(400); return; }

    char path[128];
    snprintf(path, sizeof(path), "/logs/%s", base);

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

    if (!storage_sd_available() || !server.hasArg("plain")) {
      server.send(200, "application/json", "{\"ok\":false}");
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

    char path[128];
    snprintf(path, sizeof(path), "/logs/%s", base);

    bool ok = storage_sd_fs().remove(path);
    server.send(200, "application/json",
                ok ? "{\"ok\":true}" : "{\"ok\":false}");
  });
}
