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

  // --- open, close, reopen (CRITICAL) ---
  File root = SD_MMC.open("/logs");
  if (!root || !root.isDirectory()) {
    j["ok"] = false;
    server.send(200, "application/json", j.as<String>());
    return;
  }
  root.close();
  delay(2);
  root = SD_MMC.open("/logs");

  // --- iterate ---
  while (true) {
    File f = root.openNextFile();
    if (!f) break;

    if (!f.isDirectory()) {
      JsonObject o = files.createNestedObject();
      o["name"] = String(f.name());
      o["size"] = f.size();
    }

    f.close();
  }

  root.close();

  server.send(200, "application/json", j.as<String>());
});

  // ---------------------------------------------------------------------------
  // FILE DOWNLOAD
  //
  // Streams a single validated log file to the client.
  // ---------------------------------------------------------------------------
  server.on("/api/file", HTTP_GET, [&] {
    if (!sdOK || !server.hasArg("name")) {
      server.send(404);
      return;
    }

    String name = server.arg("name");
    const char* base = basenameOnly(name.c_str());
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

    String path = "/logs/" + name;
    File f = SD_MMC.open(path, FILE_READ);
    if (!f) {
      server.send(404);
      return;
    }

    // Force browser download
    server.sendHeader("Content-Disposition","attachment; filename=\"" + name + "\"");
    server.streamFile(f, "application/octet-stream");
    f.close();
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
