#include "web_files.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD_MMC.h>

#include "system_mode.h"
#include "Storage/storage_manager.h"

// -----------------------------------------------------------------------------
// helpers (local to file API)
// -----------------------------------------------------------------------------

static const char* basenameOnly(const char* path)
{
  if (!path) return nullptr;
  const char* p = strrchr(path, '/');
  return p ? p + 1 : path;
}

static bool isValidLogFilename(const char* path)
{
  const char* name = basenameOnly(path);
  if (!name || !*name) return false;

  size_t len = strlen(name);
  if (len < 8 || len > 96) return false;
  if (name[0] == '.') return false;

  for (const char* p = name; *p; ++p) {
    char c = *p;
    if (c < 32 || c > 126) return false;
    if (c == '\'' || c == '"' || c == '\\' ||
        c == '<'  || c == '>' || c == '&')
      return false;
    if (c == '/' || c == ':')
      return false;
  }

  const char* ext = strrchr(name, '.');
  if (!ext || ext == name) return false;

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

void registerFileEndpoints(WebServer &server)
{
  // ---------------------------------------------------------------------------
  // FILE LIST
  // ---------------------------------------------------------------------------
  server.on("/api/files", HTTP_GET, [&] {

    DynamicJsonDocument j(16384);

    if (!sdOK || getMode() != MODE_WIFI_SOFT_AP) {
      j["ok"] = false;
      j["err"] = "sd_unavailable_or_not_config";
      server.send(200, "application/json", j.as<String>());
      return;
    }

    j["ok"]      = true;
    j["free_kb"] = storageFreeKBytes();
    JsonArray arr = j.createNestedArray("files");

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

      if (f.isDirectory()) { f.close(); continue; }

      char namebuf[96];
      strlcpy(namebuf, basenameOnly(f.name()), sizeof(namebuf));

      size_t size = f.size();

      if (!isValidLogFilename(namebuf)) { f.close(); continue; }
      if (size == 0 || size > (100UL * 1024UL * 1024UL)) { f.close(); continue; }

      JsonObject o = arr.createNestedObject();
      o["name"] = namebuf;
      o["size"] = size;

      f.close();
      if (++count >= 200) break;
    }

    root.close();

    if (j.overflowed()) {
      DynamicJsonDocument e(256);
      e["ok"] = false;
      e["err"] = "json_overflow";
      server.send(200, "application/json", e.as<String>());
      return;
    }

    server.send(200, "application/json", j.as<String>());
  });

  // ---------------------------------------------------------------------------
  // FILE DOWNLOAD
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

    server.sendHeader("Content-Disposition",
                      "attachment; filename=\"" + name + "\"");
    server.streamFile(f, "application/octet-stream");
    f.close();
  });

  // ---------------------------------------------------------------------------
  // FILE DELETE
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
