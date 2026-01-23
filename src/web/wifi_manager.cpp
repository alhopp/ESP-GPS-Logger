// ============================================================================
// Wi-Fi Manager (STA + AP provisioning)
// - Primary: Phone hotspot (STA)
// - Fallback: AP provisioning
// - Web server only runs when network stack is UP
// ============================================================================

#include "web/web_server.h"
#include "web/wifi_manager.h"

#include <WiFi.h>
#include <ESPmDNS.h>

#include "config_manager.h"
#include "task_display.h"

// ============================================================================
// DEV MODE OVERRIDE
// ============================================================================
// Uncomment during development to force STA and disable AP
#define DEV_FORCE_WIFI

#ifdef DEV_FORCE_WIFI
static const char *DEV_SSID = "Als_iPhone";
static const char *DEV_PASS = "alan1234";
#endif

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

static bool wifiStarted = false;
static bool apActive    = false;

// ---------------------------------------------------------------------------
// STA retry control
// ---------------------------------------------------------------------------

static unsigned long lastStaAttempt = 0;
static int           staAttempts    = 0;

#define STA_RETRY_INTERVAL_MS  5000
#define STA_MAX_ATTEMPTS       5

// ---------------------------------------------------------------------------
// UI state
// ---------------------------------------------------------------------------

static WifiUiState wifiUiState = WIFI_UI_OFF;

WifiUiState wifi_get_ui_state()
{
  return wifiUiState;
}

static void wifi_set_ui_state(WifiUiState s)
{
  if (wifiUiState == s) return;   // no-op if unchanged
  wifiUiState = s;

  // CONFIG screen content area only
  screen_request_partial(0, 0, 250, 122);
}


// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

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
#ifdef DEV_FORCE_WIFI
  return true;
#else
  return config.phone_ssid[0];
#endif
}

// ---------------------------------------------------------------------------
// STA
// ---------------------------------------------------------------------------

static void start_sta()
{
#ifdef DEV_FORCE_WIFI
  const char *ssid = DEV_SSID;
  const char *pass = DEV_PASS;
  Serial.println("[WIFI] DEV FORCE STA");
#else
  const char *ssid = config.phone_ssid;
  const char *pass = config.phone_pass;
#endif

  wifi_set_ui_state(WIFI_UI_TRYING);
  Serial.printf("[WIFI] STA connect: %s\n", ssid);

  WiFi.disconnect(true, true);
  delay(100);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(true);
  WiFi.setAutoReconnect(false);
  WiFi.begin(ssid, pass);

  lastStaAttempt = millis();
  staAttempts++;

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 3000)
    delay(100);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WIFI] Connected IP=%s\n",
      WiFi.localIP().toString().c_str());

    wifi_set_ui_state(WIFI_UI_OFF);

    if (MDNS.begin("gps"))
      MDNS.addService("http", "tcp", 80);

    webserver_start();
  } else {
    Serial.println("[WIFI] STA not connected");
    wifi_set_ui_state(WIFI_UI_FAILED);

  }
}

// ---------------------------------------------------------------------------
// AP
// ---------------------------------------------------------------------------

static void start_ap()
{
  if (apActive) return;

  Serial.println("[WIFI] Starting AP provisioning mode");

  WiFi.disconnect(true, true);
  delay(100);

  WiFi.mode(WIFI_AP);
  WiFi.softAP("GPS-Setup");

  Serial.printf("[WIFI] AP IP=%s\n",
    WiFi.softAPIP().toString().c_str());

  apActive = true;
  wifi_set_ui_state(WIFI_UI_AP);

  webserver_start();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void wifi_init()
{
  staAttempts    = 0;
  lastStaAttempt = millis();
  wifiStarted    = true;
  apActive       = false;

  if (!have_phone_wifi()) {
    Serial.println("[WIFI] No phone Wi-Fi → AP immediately");
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

  wifiStarted = false;
  apActive    = false;
  wifi_set_ui_state(WIFI_UI_OFF);

  Serial.println("[WIFI] Wi-Fi stopped");
}

void wifi_loop()
{
  if (!wifiStarted) return;
  if (apActive) return;
  if (wifi_sta_connected()) return;

  if (staAttempts >= STA_MAX_ATTEMPTS) {
#ifdef DEV_FORCE_WIFI
    Serial.println("[WIFI] DEV MODE: staying in STA retry loop");
    staAttempts = 0;
    return;
#else
    Serial.println("[WIFI] STA failed → AP fallback");
    start_ap();
    return;
#endif
  }

  if (millis() - lastStaAttempt > STA_RETRY_INTERVAL_MS) {
    Serial.printf("[WIFI] STA retry %d/%d\n",
      staAttempts + 1,
      STA_MAX_ATTEMPTS);

    start_sta();
  }
}

// ---------------------------------------------------------------------------
// Public status abstraction
// ---------------------------------------------------------------------------

bool wifi_net_active()
{
  return wifi_sta_connected() || wifi_ap_active();
}

bool wifi_show_ap_page()
{
  return apActive;
}
