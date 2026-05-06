#pragma once

#include <ArduinoJson.h>
#include <WebServer.h>

void web_send_json(WebServer& server, JsonDocument& doc);
