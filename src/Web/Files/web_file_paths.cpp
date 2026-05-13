// ============================================================================
// web_file_paths.cpp
//
// Safe filename validation and /logs path construction for web file endpoints.
// ============================================================================

#include "Web/Files/web_file_paths.h"

#include <Arduino.h>
#include <string.h>

namespace {
bool isAllowedLogExtension(const char* ext)
{
  return !strcasecmp(ext, ".sbp") ||
         !strcasecmp(ext, ".ubx") ||
         !strcasecmp(ext, ".geojson");
}
}

const char* web_log_basename(const char* path)
{
  if (!path) return nullptr;
  const char* p = strrchr(path, '/');
  return p ? p + 1 : path;
}

bool web_log_has_extension(const char* name, const char* ext)
{
  const char* actual = strrchr(name, '.');
  return actual && !strcasecmp(actual, ext);
}

bool web_log_is_valid_file(const char* name)
{
  if (!name || !*name) return false;
  if (strchr(name, '/') || strchr(name, '\\')) return false;

  const char* ext = strrchr(name, '.');
  return ext && isAllowedLogExtension(ext);
}

bool web_log_resolve_path(const char* requestedName, ValidatedLogPath& result)
{
  const char* base = web_log_basename(requestedName);
  if (!web_log_is_valid_file(base)) return false;

  result.base = base;
  web_log_build_path(result.path, sizeof(result.path), base);
  return true;
}

void web_log_build_path(char* out, size_t outSize, const char* base)
{
  snprintf(out, outSize, "/logs/%s", base);
}

void web_log_build_paired_path(char* out, size_t outSize, const char* base, const char* newExt)
{
  char stem[96];
  strlcpy(stem, base, sizeof(stem));

  char* dot = strrchr(stem, '.');
  if (dot) *dot = '\0';

  snprintf(out, outSize, "/logs/%s%s", stem, newExt);
}
