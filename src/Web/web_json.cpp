#include "Web/web_json.h"

void web_send_json(WebServer& server, JsonDocument& doc)
{
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}
