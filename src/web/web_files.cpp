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
#include <SD_MMC.h>

#include "Storage/storage_manager.h"

#include <dirent.h>
#include <sys/stat.h>

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

    DIR* dir = opendir("/sdcard/logs");
    if (!dir) {
      j["ok"] = false;
      server.send(200, "application/json", j.as<String>());
      return;
    }

    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
      if (ent->d_type != DT_REG) continue;

      String name = ent->d_name;
      String path = "/sdcard/logs/" + name;

      struct stat st;
      if (stat(path.c_str(), &st) != 0) continue;

      JsonObject o = files.createNestedObject();
      o["name"] = name;
      o["size"] = st.st_size;
    }

    closedir(dir);
    server.send(200, "application/json", j.as<String>());
  });

  // ---------------------------------------------------------------------------
  // GET /api/download?file=...
  // Stream validated log file (forced download)
  // ---------------------------------------------------------------------------
  server.on("/api/download", HTTP_GET, [&] {

    if (!sdOK || !server.hasArg("file")) {
      server.send(400);
      return;
    }

    String file = server.arg("file");

    // Strip cache-buster (?t=...)
    int q = file.indexOf('?');
    if (q >= 0) file = file.substring(0, q);

    const char* base = basenameOnly(file.c_str());
    if (!isValidLogFile(base)) { server.send(400); return; }


    char path[128];
    snprintf(path, sizeof(path), "/logs/%s", base);

    if (!SD_MMC.exists(path)) {server.send(404); return;}

    File f = SD_MMC.open(path, FILE_READ);
    if (!f) {
      server.send(500);
      return;
    }

    //   server.sendHeader("Cache-Control", "no-store");
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

    if (!sdOK || !server.hasArg("plain")) {
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

    bool ok = SD_MMC.remove(path);
    server.send(200, "application/json",
                ok ? "{\"ok\":true}" : "{\"ok\":false}");
  });
}
