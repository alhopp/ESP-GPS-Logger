#pragma once

#include <Arduino.h>
#include <webserver.h>
// -----------------------------------------------------------------------------
// Wi-Fi lifecycle
// -----------------------------------------------------------------------------

// Decide initial Wi-Fi mode at boot (HOME or FIELD_CONFIG)
void initWifi();

// Start Wi-Fi in STA (HOME) mode
void wifi_start_sta();

// Start Wi-Fi in AP (FIELD_CONFIG) mode
void wifi_start_ap();

// Stop all Wi-Fi activity
void wifi_stop();

// Service Wi-Fi / web server loop
void wifi_loop();

// Factory reset Wi-Fi credentials
void wifi_factory_reset();

// -----------------------------------------------------------------------------
// Wi-Fi configuration
// -----------------------------------------------------------------------------

// Update stored HOME Wi-Fi credentials
// (persistence handled internally)
void wifi_set_credentials(const String& ssid, const String& pass);

// Return AP SSID (for display / QR / UI)
const char* wifi_ap_name();

bool   wifi_is_ap_mode();
String wifi_get_scan_json();

// -----------------------------------------------------------------------------
// Saved Wi-Fi credentials access (for Web UI autofill)
// -----------------------------------------------------------------------------

bool   wifi_has_credentials();
String wifi_get_saved_ssid();
String wifi_get_saved_pass();

// -----------------------------------------------------------------------------
// Live STA status (runtime)
// -----------------------------------------------------------------------------
String wifi_sta_ssid();
String wifi_sta_ip();

