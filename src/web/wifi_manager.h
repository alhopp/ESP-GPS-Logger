#pragma once

#include <Arduino.h>

// ============================================================================
// Wi-Fi lifecycle (STA-only)
// ============================================================================

// Initialise Wi-Fi at boot (STA-only)
void wifi_init();

// Stop Wi-Fi completely
void wifi_stop();

// Service web / Wi-Fi loop
void wifi_loop();

void webserver_start();

// ============================================================================
// STA status helpers
// ============================================================================

bool   wifi_sta_connected();
String wifi_sta_ssid();
String wifi_sta_ip();
