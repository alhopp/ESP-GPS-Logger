#pragma once

#include <WebServer.h>

// Registers lightweight status endpoints used by the config UI.
void registerStatusApi(WebServer& server);
