#pragma once

// ============================================================================
// api_config.h
//
// Public registration point for the web configuration endpoint.
// ============================================================================

#include <WebServer.h>

// Registers GET/POST /api/config.
//
// GET returns system facts plus editable config groups. POST accepts the same
// nested groups used by data/config.js and persists them via saveConfig().
void registerConfigApi(WebServer& server);
