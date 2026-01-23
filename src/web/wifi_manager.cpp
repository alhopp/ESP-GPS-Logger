// ============================================================================
// Wi-Fi Manager (STA-only, PHONE hotspot only)
// - Uses phone_ssid / phone_pass from config
// - No AP mode
// - No home Wi-Fi
// - Simple retry logic
// ============================================================================

#include "web/web_server.h"
#include "web/wifi_manager.h"

#include <WiFi.h>
#include <ESPmDNS.h>

#include "config_manager.h"

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

static bool wifiStarted = false;

// ---------------------------------------------------------------------------
// STA retry control
// ---------------------------------------------------------------------------

static unsigned long lastStaAttempt = 0;
static int           staAttempts    = 0;

#define STA_RETRY_INTERVAL_MS  5000   // 5 s
#define STA_MAX_ATTEMPTS       10

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static bool have_phone_wifi()
{
  return config.phone_ssid[0];
}

static void start_sta()
{
  if (!have_phone_wifi()) {
    Serial.println("[WIFI] Phone hotspot SSID not set → Wi-Fi disabled");
    return;
  }

  Serial.printf("[WIFI] STA connect (phone): %s\n", config.phone_ssid);

  WiFi.disconnect(true, true);
  delay(100);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(true);
  WiFi.setAutoReconnect(false);

  WiFi.begin(config.phone_ssid, config.phone_pass);

  lastStaAttempt = millis();
  staAttempts++;

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 3000) {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WIFI] Connected IP=%s\n",
      WiFi.localIP().toString().c_str());

    if (MDNS.begin("gps")) {
      MDNS.addService("http", "tcp", 80);
    }
  } else {
    Serial.println("[WIFI] STA not connected");
  }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void wifi_init()
{
  staAttempts    = 0;
  lastStaAttempt = millis();

  if (!have_phone_wifi()) {
    Serial.println("[WIFI] No phone Wi-Fi configured → Wi-Fi OFF");
    return;
  }

  start_sta();
  wifiStarted = true;
}

void wifi_stop()
{
  if (!wifiStarted) return;

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  wifiStarted = false;
  Serial.println("[WIFI] Wi-Fi stopped");
}

void wifi_loop()
{
  if (!wifiStarted) return;

  // Already connected → nothing to do
  if (WiFi.status() == WL_CONNECTED) return;

  // Give up quietly after max retries
  if (staAttempts >= STA_MAX_ATTEMPTS) return;

  // Retry STA connection
  if (millis() - lastStaAttempt > STA_RETRY_INTERVAL_MS) {
    Serial.printf("[WIFI] STA retry %d/%d\n",
      staAttempts + 1,
      STA_MAX_ATTEMPTS);

    start_sta();
  }
}

// ---------------------------------------------------------------------------
// STA helpers
// ---------------------------------------------------------------------------

bool wifi_sta_connected()
{
  return WiFi.status() == WL_CONNECTED;
}

String wifi_sta_ssid()
{
  return WiFi.isConnected() ? WiFi.SSID() : String();
}

String wifi_sta_ip()
{
  return WiFi.isConnected() ? WiFi.localIP().toString() : String();
}
