#pragma once

// ============================================================================
// api_status.h
//
// Public registration point for web runtime status endpoints.
// ============================================================================

#include <WebServer.h>

// Registers lightweight status endpoints used by the config UI.
void registerStatusApi(WebServer& server);
