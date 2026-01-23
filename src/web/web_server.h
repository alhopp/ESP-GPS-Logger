#pragma once

#include <WebServer.h>

// -----------------------------------------------------------------------------
// Web server lifecycle
// -----------------------------------------------------------------------------
void webserver_start();
void webserver_stop();
void webserver_loop();



// -----------------------------------------------------------------------------
// STA network status (used by UI + logs + display)
// -----------------------------------------------------------------------------

bool   wifi_sta_connected();
String wifi_sta_ssid();
String wifi_sta_ip();
