#pragma once

// ============================================================================
// web_json.h
//
// Shared JSON response helper used by web API handlers.
// ============================================================================

#include <ArduinoJson.h>
#include <WebServer.h>

// Serialize a JSON document with the standard content type.
void web_send_json(WebServer& server, JsonDocument& doc);
