// ============================================================================
// wifi_credentials.cpp
//
// Selects saved or build-time phone hotspot credentials for STA mode.
// ============================================================================

#include "Web/WiFi/wifi_credentials.h"

#include "Config/config_types.h"
#include "Core/build_config.h"

#if DEV_FORCE_WIFI
namespace {
const char* DEV_SSID = "Als_iPhone";
const char* DEV_PASS = "alan1234";
}
#endif

WifiCredentials wifi_effective_phone_credentials()
{
#if DEV_FORCE_WIFI
  return {DEV_SSID, DEV_PASS};
#else
  return {config.phone_ssid, config.phone_pass};
#endif
}

const char* wifi_effective_phone_ssid()
{
  return wifi_effective_phone_credentials().ssid;
}

bool wifi_effective_phone_password_set()
{
  return wifi_effective_phone_credentials().pass[0];
}
