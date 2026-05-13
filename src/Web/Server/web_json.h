#pragma once

#include <ArduinoJson.h>
#include <WebServer.h>

// Serialize a JSON document with the standard content type.
void web_send_json(WebServer& server, JsonDocument& doc);
