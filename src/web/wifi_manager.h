#pragma once

#include <Arduino.h>

// ============================================================================
// Wi-Fi lifecycle
// ============================================================================

// Initialise Wi-Fi at boot
// - First boot / no creds → AP mode
// - Otherwise STA with fallback to AP
void wifi_init();

// Stop Wi-Fi completely (STA or AP)
void wifi_stop();

// Service Wi-Fi state machine (called from loop)
void wifi_loop();

// Exit AP setup mode (called from web UI)
void wifi_exit_ap();

// ============================================================================
// Status helpers
// ============================================================================

// STA status
bool   wifi_sta_connected();
String wifi_sta_ssid();
String wifi_sta_ip();

// AP status
bool   wifi_ap_active();
