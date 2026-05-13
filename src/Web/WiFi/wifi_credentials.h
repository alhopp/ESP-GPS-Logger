#pragma once

// ============================================================================
// wifi_credentials.h
//
// Effective phone hotspot credentials used by Wi-Fi STA mode. Dev builds may
// override saved config credentials here without cluttering the Wi-Fi state
// machine.
// ============================================================================

struct WifiCredentials {
  const char* ssid;
  const char* pass;
};

WifiCredentials wifi_effective_phone_credentials();
const char* wifi_effective_phone_ssid();
bool wifi_effective_phone_password_set();
