// ============================================================================
// Wi-Fi Manager (STA + AP provisioning)
// - Primary: Phone hotspot (STA)
// - Fallback: AP provisioning
// - Web server only runs when network stack is UP
// ============================================================================

#include "Web/WiFi/wifi_manager.h"

#include <ESPmDNS.h>
#include <WiFi.h>

#include "Core/build_config.h"
#include "Core/log.h"
#include "Runtime/Display/display_redraw.h"

namespace {

// -----------------------------------------------------------------------------
// State
// -----------------------------------------------------------------------------

bool wifiStarted = false;
bool apActive = false;
bool mdnsStarted = false;

// -----------------------------------------------------------------------------
// STA retry control
// -----------------------------------------------------------------------------

uint32_t lastStaAttempt = 0;
uint32_t lastStaConnected = 0;
uint32_t staSearchStarted = 0;
int staAttempts = 0;

constexpr uint32_t STA_RETRY_INTERVAL_MS = 5000;
constexpr uint32_t STA_LOST_GRACE_MS = 5000;
constexpr uint32_t STA_HOTSPOT_WAIT_MS = 60000;
constexpr const char* AP_PROVISIONING_SSID = "GPS-Setup";

// -----------------------------------------------------------------------------
// UI state
// -----------------------------------------------------------------------------

WifiUiState wifiUiState = WIFI_UI_OFF;
constexpr DisplayWindow WIFI_STATUS_WINDOW = DISPLAY_FULL_WINDOW;

void wifi_set_ui_state(WifiUiState s)
{
  if (wifiUiState == s) return;

  wifiUiState = s;
  screen_request_partial(WIFI_STATUS_WINDOW);
}

// -----------------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------------

bool wifi_sta_connected()
{
  return WiFi.status() == WL_CONNECTED;
}

bool wifi_ap_active()
{
  return apActive;
}

void disconnect_wifi_radios()
{
  WiFi.disconnect(true, true);
  delay(100);
}

void stop_mdns()
{
  if (!mdnsStarted) return;
  MDNS.end();
  mdnsStarted = false;
}

void reset_sta_retry_state()
{
  staAttempts = 0;
  const uint32_t now = millis();
  staSearchStarted = now;
  lastStaAttempt = now;
  lastStaConnected = 0;
}

bool have_phone_wifi()
{
  return wifi_effective_phone_ssid()[0];
}

bool phone_hotspot_visible(const char* ssid)
{
  if (!ssid || !ssid[0]) return false;

  WiFi.mode(WIFI_STA);
  const int networkCount = WiFi.scanNetworks(false, true);
  bool found = false;

  for (int i = 0; i < networkCount; i++) {
    if (WiFi.SSID(i) == ssid) {
      found = true;
      break;
    }
  }

  WiFi.scanDelete();
  return found;
}

void mark_sta_connected()
{
  LOG_WIFI("STA", "connected IP=%s", WiFi.localIP().toString().c_str());
  lastStaConnected = millis();
  staAttempts = 0;
  wifi_set_ui_state(WIFI_UI_CONNECTED);

  if (!mdnsStarted && MDNS.begin("gps")) {
    MDNS.addService("http", "tcp", 80);
    mdnsStarted = true;
  }
}

// -----------------------------------------------------------------------------
// STA
// -----------------------------------------------------------------------------

void start_sta()
{
  if (wifi_sta_connected()) {
    LOG_WIFI("STA", "already connected IP=%s", WiFi.localIP().toString().c_str());
    mark_sta_connected();
    return;
  }

  const WifiCredentials phoneWifi = wifi_effective_phone_credentials();
#if DEV_FORCE_WIFI
  LOG_WIFI("STA", "DEV FORCE");
#endif

  wifi_set_ui_state(WIFI_UI_TRYING);
  disconnect_wifi_radios();
  WiFi.mode(WIFI_STA);

  if (!phone_hotspot_visible(phoneWifi.ssid)) {
    LOG_WIFI("STA", "waiting for hotspot %s", phoneWifi.ssid);
    lastStaAttempt = millis();
    staAttempts++;
    return;
  }

  LOG_WIFI("STA", "connect %s", phoneWifi.ssid);

  WiFi.setSleep(true);
  WiFi.setAutoReconnect(false);
  WiFi.begin(phoneWifi.ssid, phoneWifi.pass);

  lastStaAttempt = millis();
  staAttempts++;
}

bool hotspotWaitExpired()
{
  return staSearchStarted && millis() - staSearchStarted >= STA_HOTSPOT_WAIT_MS;
}

// -----------------------------------------------------------------------------
// AP
// -----------------------------------------------------------------------------

void start_ap()
{
  if (apActive) return;

  LOG_WIFI("AP", "starting provisioning mode");

  disconnect_wifi_radios();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_PROVISIONING_SSID);

  LOG_WIFI("AP", "IP=%s", WiFi.softAPIP().toString().c_str());

  apActive = true;
  wifi_set_ui_state(WIFI_UI_AP);
}

void handle_sta_connected()
{
  if (wifiUiState != WIFI_UI_CONNECTED) {
    mark_sta_connected();
    return;
  }

  lastStaConnected = millis();
}

void handle_sta_lost()
{
  LOG_WIFI("STA", "connection lost, waiting");
  wifi_set_ui_state(WIFI_UI_TRYING);
  lastStaAttempt = millis();
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

} // namespace

WifiUiState wifi_get_ui_state()
{
  return wifiUiState;
}

void wifi_init()
{
  reset_sta_retry_state();
  wifiStarted = true;
  apActive = false;

  if (!have_phone_wifi()) {
    LOG_WIFI("Init", "no phone Wi-Fi, AP immediately");
    start_ap();
    return;
  }

  start_sta();
}

void wifi_stop()
{
  if (!wifiStarted) return;

  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  stop_mdns();

  wifiStarted = false;
  apActive = false;
  wifi_set_ui_state(WIFI_UI_OFF);

  LOG_WIFI("Stop", "Wi-Fi stopped");
}

void wifi_loop()
{
  if (!wifiStarted) return;
  if (apActive) return;
  if (wifi_sta_connected()) {
    handle_sta_connected();
    return;
  }

  if (wifiUiState == WIFI_UI_CONNECTED) {
    handle_sta_lost();
    return;
  }

  if (lastStaConnected && millis() - lastStaConnected <= STA_LOST_GRACE_MS) {
    return;
  }

  if (millis() - lastStaAttempt <= STA_RETRY_INTERVAL_MS) return;

  if (hotspotWaitExpired() && !build_dev_wifi_enabled()) {
    LOG_WIFI("STA", "hotspot not found, AP fallback");
    wifi_set_ui_state(WIFI_UI_FAILED);
    start_ap();
    return;
  }

  LOG_WIFI("STA", "retry %d", staAttempts + 1);
  start_sta();
}

// -----------------------------------------------------------------------------
// Public status abstraction
// -----------------------------------------------------------------------------

bool wifi_net_active()
{
  return wifi_sta_connected() || wifi_ap_active();
}

bool wifi_show_ap_page()
{
  return apActive;
}
