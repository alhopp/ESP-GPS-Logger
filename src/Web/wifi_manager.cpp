// ============================================================================
// Wi-Fi Manager (STA + AP provisioning)
// - Primary: Phone hotspot (STA)
// - Fallback: AP provisioning
// - Web server only runs when network stack is UP
// ============================================================================

#include "Web/web_server.h"
#include "Web/wifi_manager.h"

#include <ESPmDNS.h>
#include <WiFi.h>

#include "Core/build_config.h"
#include "Core/log.h"
#include "Config/config_types.h"
#include "Runtime/display_redraw.h"

// ============================================================================
// DEV MODE OVERRIDE
// ============================================================================
#if DEV_FORCE_WIFI
static const char *DEV_SSID = "Als_iPhone";
static const char *DEV_PASS = "alan1234";
#endif

// -----------------------------------------------------------------------------
// State
// -----------------------------------------------------------------------------

static bool wifiStarted = false;
static bool apActive = false;
static bool mdnsStarted = false;

// -----------------------------------------------------------------------------
// STA retry control
// -----------------------------------------------------------------------------

static uint32_t lastStaAttempt = 0;
static int staAttempts = 0;

static constexpr uint32_t STA_RETRY_INTERVAL_MS = 3000;
static constexpr int STA_MAX_ATTEMPTS = 10;

// -----------------------------------------------------------------------------
// UI state
// -----------------------------------------------------------------------------

static WifiUiState wifiUiState = WIFI_UI_OFF;
static constexpr DisplayWindow WIFI_STATUS_WINDOW = DISPLAY_FULL_WINDOW;

WifiUiState wifi_get_ui_state()
{
  return wifiUiState;
}

static void wifi_set_ui_state(WifiUiState s)
{
  if (wifiUiState == s) return;

  wifiUiState = s;
  screen_request_partial(WIFI_STATUS_WINDOW);
}

// -----------------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------------

static bool wifi_sta_connected()
{
  return WiFi.status() == WL_CONNECTED;
}

static bool wifi_ap_active()
{
  return apActive;
}

static bool have_phone_wifi()
{
  if (build_dev_wifi_enabled()) {
    return true;
  }

  return config.phone_ssid[0];
}

// -----------------------------------------------------------------------------
// STA
// -----------------------------------------------------------------------------

static void start_sta()
{
  if (wifi_sta_connected()) {
    LOG_WIFI("STA", "already connected IP=%s", WiFi.localIP().toString().c_str());
    wifi_set_ui_state(WIFI_UI_CONNECTED);
    webserver_start();
    return;
  }

#if DEV_FORCE_WIFI
  const char *ssid = DEV_SSID;
  const char *pass = DEV_PASS;
  LOG_WIFI("STA", "DEV FORCE");
#else
  const char *ssid = config.phone_ssid;
  const char *pass = config.phone_pass;
#endif

  wifi_set_ui_state(WIFI_UI_TRYING);
  LOG_WIFI("STA", "connect %s", ssid);

  WiFi.disconnect(true, true);
  delay(100);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(true);
  WiFi.setAutoReconnect(false);
  WiFi.begin(ssid, pass);

  lastStaAttempt = millis();
  staAttempts++;

  const uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 3000) {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    LOG_WIFI("STA", "connected IP=%s", WiFi.localIP().toString().c_str());
    wifi_set_ui_state(WIFI_UI_CONNECTED);

    if (!mdnsStarted && MDNS.begin("gps")) {
      MDNS.addService("http", "tcp", 80);
      mdnsStarted = true;
    }

    webserver_start();
  } else {
    LOG_WIFI("STA", "not connected");
    wifi_set_ui_state(WIFI_UI_FAILED);
  }
}

// -----------------------------------------------------------------------------
// AP
// -----------------------------------------------------------------------------

static void start_ap()
{
  if (apActive) return;

  LOG_WIFI("AP", "starting provisioning mode");

  WiFi.disconnect(true, true);
  delay(100);

  WiFi.mode(WIFI_AP);
  WiFi.softAP("GPS-Setup");

  LOG_WIFI("AP", "IP=%s", WiFi.softAPIP().toString().c_str());

  apActive = true;
  wifi_set_ui_state(WIFI_UI_AP);

  webserver_start();
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void wifi_init()
{
  staAttempts = 0;
  lastStaAttempt = millis();
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

  webserver_stop();

  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  if (mdnsStarted) {
    MDNS.end();
    mdnsStarted = false;
  }

  wifiStarted = false;
  apActive = false;
  wifi_set_ui_state(WIFI_UI_OFF);

  LOG_WIFI("Stop", "Wi-Fi stopped");
}

void wifi_loop()
{
  if (!wifiStarted) return;
  if (apActive) return;
  if (wifi_sta_connected()) return;

  if (staAttempts >= STA_MAX_ATTEMPTS) {
    if (build_dev_wifi_enabled()) {
      LOG_WIFI("STA", "DEV retry loop");
      staAttempts = 0;
      return;
    }

    LOG_WIFI("STA", "failed, AP fallback");
    start_ap();
    return;
  }

  if (millis() - lastStaAttempt > STA_RETRY_INTERVAL_MS) {
    LOG_WIFI("STA", "retry %d/%d", staAttempts + 1, STA_MAX_ATTEMPTS);
    start_sta();
  }
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
