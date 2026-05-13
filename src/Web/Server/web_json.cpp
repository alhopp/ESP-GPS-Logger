// ============================================================================
// web_json.cpp
//
// Shared JSON response helper for web APIs. Serializes ArduinoJson documents
// with the standard content type and marks web activity for config timeout.
// ============================================================================

#include "Web/Server/web_json.h"

#include "Web/Server/web_server.h"

void web_send_json(WebServer& server, JsonDocument& doc)
{
  webserver_note_activity();

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}
