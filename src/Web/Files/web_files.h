#pragma once

// ============================================================================
// web_files.h
//
// Public registration point for SD /logs file APIs: list, download, and delete.
// ============================================================================

#include <WebServer.h>

void registerFileEndpoints(WebServer &server);
