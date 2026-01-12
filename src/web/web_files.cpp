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

// Validate a log filename before exposing it to the UI or filesystem.
//
// Purpose:
// - Filter out corrupted / garbage FAT entries
// - Ensure only files created by this firmware are visible
// - Prevent path traversal, HTML/JS injection, and JSON corruption
//
// Rules enforced:
// - Basename only (no paths)
// - Sensible filename length
// - No hidden or dot-files
// - Printable, UI-safe ASCII characters only
// - Exactly one extension
// - Extension must be one of the supported log formats
static bool isValidLogFilename(const char* path)
{
  const char* name = basenameOnly(path);
  if (!name || !*name) return false;

  // Enforce reasonable filename length (timestamps + MAC, etc.)
  size_t len = strlen(name);
  if (len < 8 || len > 96) return false;

  // Reject hidden files and dot entries
  if (name[0] == '.') return false;

  // Validate each character for UI / JSON safety
  for (const char* p = name; *p; ++p) {
    char c = *p;

    // Printable ASCII only
    if (c < 32 || c > 126) return false;

    // Characters that can break HTML / JS / JSON contexts
    if (c == '\'' || c == '"' || c == '\\' ||
        c == '<'  || c == '>' || c == '&')
      return false;

    // Disallow path separators (defensive)
    if (c == '/' || c == ':')
      return false;
  }

  // Must have exactly one extension
  const char* ext = strrchr(name, '.');
  if (!ext || ext == name) return false;

  // Only allow known log file extensions
  if (strcmp(ext, ".txt") &&
      strcmp(ext, ".sbp") &&
      strcmp(ext, ".ubx") &&
      strcmp(ext, ".gpx") &&
      strcmp(ext, ".gpy")) {
    return false;
  }

  return true;
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

    // Large response → heap allocation (prevents stack overflow / reboot)
    DynamicJsonDocument j(16384);

    // Only allow access if SD is present and we are in CONFIG (SoftAP) mode
    if (!sdOK || getMode() != MODE_WIFI_SOFT_AP) {
      j["ok"]  = false;
      j["err"] = "sd_unavailable_or_not_config";
      server.send(200, "application/json", j.as<String>());
      return;
    }

    j["ok"]      = true;
    j["free_kb"] = storageFreeKBytes();
    JsonArray arr = j.createNestedArray("files");

    // Open /logs directory only
    File root = SD_MMC.open("/logs");
    if (!root || !root.isDirectory()) {
      server.send(200, "application/json", j.as<String>());
      return;
    }

    root.rewindDirectory();

    int count = 0;
    while (true) {
      File f = root.openNextFile();
      if (!f) break;

      // Skip directories
      if (f.isDirectory()) {
        f.close();
        continue;
      }

      // Copy filename immediately (File.name() buffer is transient)
      char namebuf[96];
      strlcpy(namebuf, basenameOnly(f.name()), sizeof(namebuf));

      size_t size = f.size();

      // Strict validation and sanity checks
      if (!isValidLogFilename(namebuf)) { f.close(); continue; }
      if (size == 0 || size > (100UL * 1024UL * 1024UL)) { f.close(); continue; }

      JsonObject o = arr.createNestedObject();
      o["name"] = namebuf;
      o["size"] = size;

      f.close();

      // Hard cap to avoid runaway JSON growth
      if (++count >= 200) break;
    }

    root.close();

    // Detect JSON overflow explicitly
    if (j.overflowed()) {
      DynamicJsonDocument e(256);
      e["ok"]  = false;
      e["err"] = "json_overflow";
      server.send(200, "application/json", e.as<String>());
      return;
    }

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
    if (!isValidLogFilename(name.c_str())) {
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
    server.sendHeader("Content-Disposition",
                      "attachment; filename=\"" + name + "\"");
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
    if (!name || !isValidLogFilename(name)) {
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
