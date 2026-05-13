#pragma once

// ============================================================================
// web_file_paths.h
//
// Validation and path-building helpers for SD log file web APIs. Keeps request
// filenames confined to /logs and to known log/export extensions.
// ============================================================================

#include <stddef.h>

struct ValidatedLogPath {
  const char* base;
  char path[128];
};

const char* web_log_basename(const char* path);
bool web_log_has_extension(const char* name, const char* ext);
bool web_log_is_valid_file(const char* name);
bool web_log_resolve_path(const char* requestedName, ValidatedLogPath& result);

void web_log_build_path(char* out, size_t outSize, const char* base);
void web_log_build_paired_path(char* out, size_t outSize, const char* base, const char* newExt);
