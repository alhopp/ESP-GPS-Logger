// ============================================================================
// Wi-Fi Manager (STA-only, hard-coded creds)
// ============================================================================

#include "web/wifi_manager.h"

#include <WiFi.h>
#include <ESPmDNS.h>

#include "web/web_server.h"

// ---------------------------------------------------------------------------
// HARD-CODED STA CREDENTIALS (temporary)
// ---------------------------------------------------------------------------

//#define WIFI_STA_SSID "Optus_262611"
//#define WIFI_STA_PASS "lyres42527mj"

#define WIFI_STA_SSID "Als_iPhone"
#define WIFI_STA_PASS "alan1234"
// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

static bool wifiStarted = false;


// ---------------------------------------------------------------------------
// STA retry control
// ---------------------------------------------------------------------------

static unsigned long lastStaAttempt = 0;
static int           staAttempts    = 0;

#define STA_RETRY_INTERVAL_MS  5000   // retry every 15s
#define STA_MAX_ATTEMPTS       10       // then give up quietly


// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void wifi_init()
{
  Serial.printf("[WIFI] STA connect: %s\n", WIFI_STA_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(true);
  WiFi.setAutoReconnect(false);

  staAttempts    = 0;
  lastStaAttempt = millis();

  WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASS);
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
    Serial.println("[WIFI] STA not connected (will retry)");
  }

  wifiStarted = true;
}



void wifi_stop()
{
  if (!wifiStarted) return;

  webserver_stop();

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  wifiStarted = false;
}

void wifi_loop()
{
  if (!wifiStarted) return;

  // Already connected → nothing to do
  if (WiFi.status() == WL_CONNECTED) return;

  // Give up after max retries
  if (staAttempts >= STA_MAX_ATTEMPTS) return;

  // Retry STA connection
  if (millis() - lastStaAttempt > STA_RETRY_INTERVAL_MS) {
    lastStaAttempt = millis();
    staAttempts++;

    Serial.printf(
      "[WIFI] STA retry %d/%d\n",
      staAttempts,
      STA_MAX_ATTEMPTS
    );

    WiFi.disconnect(false);
    WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASS);
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
