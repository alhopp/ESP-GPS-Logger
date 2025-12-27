// ============================================================================
// Wi-Fi Manager
//  - STA if creds exist
//  - AP captive portal if not
//  - Wi-Fi scan ONCE when AP starts
// ============================================================================

#include <ArduinoJson.h>
#include "wifi_manager.h"
#include "system_mode.h"

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>

#include "Definitions.h"
#include "web/web_server.h"

// ============================================================================
// Configuration
// ============================================================================

static const char* HOSTNAME = "esp32-gps";

// FIELD CONFIG (AP)
static const char* AP_SSID = "ESP32 GPS";
static const char* AP_PASS = "12345678";   // iOS requires ≥8 chars

static IPAddress apIP(192,168,4,1);
static IPAddress netMask(255,255,255,0);

#define WIFI_FILE "/wifi.txt"

// ============================================================================
// State
// ============================================================================

static WebServer server(80);
static DNSServer dnsServer;

static bool serverStarted = false;
static bool apMode        = false;

static String savedSSID;
static String savedPASS;

// Cached scan results (FIELD mode only)
static bool scanDone = false;
static String scanJSON;

// ============================================================================
// Public helpers
// ============================================================================

const char* wifi_ap_name()
{
  return AP_SSID;
}

bool wifi_is_ap_mode()
{
  return apMode;
}

String wifi_get_scan_json()
{
  return scanJSON;
}

// ============================================================================
// Credential storage
// ============================================================================

static bool loadCreds()
{
  if (!LittleFS.exists(WIFI_FILE)) return false;

  File f = LittleFS.open(WIFI_FILE, "r");
  if (!f) return false;

  savedSSID = f.readStringUntil('\n');
  savedPASS = f.readStringUntil('\n');
  f.close();

  savedSSID.trim();
  savedPASS.trim();

  return !savedSSID.isEmpty();
}

static void saveCreds(const String& ssid, const String& pass)
{
  File f = LittleFS.open(WIFI_FILE, "w");
  if (!f) return;

  f.println(ssid);
  f.println(pass);
  f.close();
}

void wifi_set_credentials(const String& ssid, const String& pass)
{
  saveCreds(ssid, pass);
}

// ============================================================================
// Boot-time decision
// ============================================================================

void initWifi()
{
  if (loadCreds()) setMode(MODE_HOME);
  else             setMode(MODE_FIELD_CONFIG);
}

// ============================================================================
// Web server lifecycle
// ============================================================================

static void startServer()
{
  if (serverStarted) return;
  webserver_start(server);
  serverStarted = true;
}

// ============================================================================
// STA mode (NO SCANS EVER)
// ============================================================================

void wifi_start_sta()
{
  wifi_stop();

  if (!loadCreds()) {
    setMode(MODE_FIELD_CONFIG);
    return;
  }

  apMode   = false;
  scanDone = false;
  scanJSON = "";

  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPASS.c_str());

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 3000) {
    delay(50);
  }

  if (WiFi.status() != WL_CONNECTED) {
    setMode(MODE_FIELD_CONFIG);
    return;
  }

  if (MDNS.begin(HOSTNAME)) {
    MDNS.addService("http","tcp",80);
  }

  startServer();
}

// ============================================================================
// AP + Captive Portal (SCAN ONCE HERE)
// ============================================================================

static void scanOnce()
{
  if (scanDone) return;

  StaticJsonDocument<2048> j;
  JsonArray a = j.to<JsonArray>();

  int n = WiFi.scanNetworks(false, false);

  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    if (!ssid.length()) continue;

    bool dup = false;
    for (JsonObject o : a)
      if (o["ssid"] == ssid) { dup = true; break; }

    if (dup) continue;

    JsonObject o = a.createNestedObject();
    o["ssid"]   = ssid;
    o["rssi"]   = WiFi.RSSI(i);
    o["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
  }

  WiFi.scanDelete();
  serializeJson(j, scanJSON);
  scanDone = true;
}

void wifi_start_ap()
{
  wifi_stop();

  apMode   = true;
  scanDone = false;
  scanJSON = "";

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMask);
  WiFi.softAP(AP_SSID, AP_PASS);

  unsigned long t0 = millis();
  while (WiFi.softAPIP() == IPAddress(0,0,0,0) && millis() - t0 < 500)
    delay(10);

  dnsServer.start(53, "*", WiFi.softAPIP());

  scanOnce();          // ⭐ ONE AND ONLY SCAN
  startServer();
}

// ============================================================================
// Stop Wi-Fi
// ============================================================================

void wifi_stop()
{
  if (!serverStarted && WiFi.getMode() == WIFI_OFF) return;

  webserver_stop();
  server.stop();
  dnsServer.stop();

  WiFi.disconnect(true,true);
  WiFi.mode(WIFI_OFF);

  serverStarted = false;
  apMode        = false;
}

// ============================================================================
// Loop service
// ============================================================================

void wifi_loop()
{
  if (!serverStarted) return;

  server.handleClient();

  if (apMode)
    dnsServer.processNextRequest();
}

// ============================================================================
// Factory reset
// ============================================================================

void wifi_factory_reset()
{
  if (LittleFS.exists(WIFI_FILE))
    LittleFS.remove(WIFI_FILE);
}
