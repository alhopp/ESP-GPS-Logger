// ============================================================================
// web_files.cpp
//
// Web-facing SD card file API for the ESP32 GPS Logger.
//
// Responsibilities:
// - Expose read-only access to log files stored on SD (/logs)
// - Provide safe, validated endpoints for:
//     • Listing log files
//     • Downloading individual files
//     • Deleting files (explicit user action)
//
// Design & safety notes:
// - All file operations are restricted to the /logs directory
// - Filenames are aggressively validated before *any* filesystem access
// - Directory enumeration is capped to avoid heap / JSON exhaustion
// - Endpoints are only active while in CONFIG (SoftAP) mode
//
// ESP32-specific notes:
// - File.name() returns a transient internal buffer → copy immediately
// - Large JSON responses must live on the heap (DynamicJsonDocument)
// ============================================================================

#include "web_files.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD_MMC.h>

#include "system_mode.h"
#include "Storage/storage_manager.h"

#include <dirent.h>     // DIR, opendir, readdir, closedir, struct dirent
#include <sys/stat.h>   // stat()


// -----------------------------------------------------------------------------
// Helper utilities (local to this translation unit)
// -----------------------------------------------------------------------------

// Extract the basename (filename only) from a full filesystem path.
//
// Example:
//   "/logs/run_20260110.sbp" → "run_20260110.sbp"
//
// Notes:
// - Returned pointer refers to the input string
// - Callers MUST copy the result immediately if persistence is required
static const char* basenameOnly(const char* path)
{
  if (!path) return nullptr;
  const char* p = strrchr(path, '/');
  return p ? p + 1 : path;
}



// -----------------------------------------------------------------------------
// API registration
// -----------------------------------------------------------------------------

// Register all file-related HTTP endpoints on the provided WebServer instance.
void registerFileEndpoints(WebServer &server)
{
  // ---------------------------------------------------------------------------
  // FILE LIST
  //
  // Returns a JSON array of valid log files found in /logs.
  // Includes filename and size only.
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

    if (ent->d_type != DT_REG)
      continue;

    String name = ent->d_name;
    String path = "/sdcard/logs/" + name;

    struct stat st;
    if (stat(path.c_str(), &st) != 0)
      continue;

    JsonObject o = files.createNestedObject();
    o["name"] = name;
    o["size"] = st.st_size;
  }

  closedir(dir);

  server.send(200, "application/json", j.as<String>());
});

  // ---------------------------------------------------------------------------
  // FILE DELETE
  //
  // Deletes a single validated log file from /logs.
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

    const char* name = j["name"];
    if (!name) {
      server.send(400);
      return;
    }

    const char* base = basenameOnly(name);
    const char* ext  = strrchr(base, '.');

    if (!ext ||
        (strcasecmp(ext, ".txt") &&
        strcasecmp(ext, ".sbp") &&
        strcasecmp(ext, ".ubx") &&
        strcasecmp(ext, ".gpx") &&
        strcasecmp(ext, ".gpy"))) {
      server.send(400);
      return;
    }


    char path[128];
    snprintf(path, sizeof(path), "/logs/%s", name);

    bool ok = SD_MMC.remove(path);
    server.send(200, "application/json",
                ok ? "{\"ok\":true}" : "{\"ok\":false}");
  });
}
