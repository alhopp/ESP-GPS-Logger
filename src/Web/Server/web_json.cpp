#include "Web/Server/web_json.h"

#include "Web/Server/web_server.h"

void web_send_json(WebServer& server, JsonDocument& doc)
{
  webserver_note_activity();

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}
