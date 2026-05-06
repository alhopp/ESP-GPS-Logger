#pragma once

// ============================================================================
// web_server.h
//
// HTTP server lifecycle.
//
// The server is started only when Wi-Fi is active. Endpoint registration lives
// behind webserver_start(); callers should not access the global WebServer.
// ============================================================================

void webserver_start();
void webserver_stop();
void webserver_loop();
