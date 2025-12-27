// ============================================================================
// Wi-Fi Manager
//
// Behaviour:
//  - If valid Wi-Fi credentials exist → try STA (HOME)
//  - If STA fails or no credentials → AP captive portal (FIELD_CONFIG)
//  - Field portal allows:
//      • Editing HOME Wi-Fi credentials
//      • Editing system variables
//
// Design principles:
//  - Wi-Fi never blocks boot
//  - Wi-Fi never bricks device
//  - UI / HTML is delegated to web_server.cpp
// ============================================================================

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

// Access Point (FIELD_CONFIG mode)
static const char* AP_SSID = "ESP32 GPS";
static const char* AP_PASS = "12345678";   // iOS requires ≥ 8 chars

static IPAddress apIP(192, 168, 4, 1);
static IPAddress netMask(255, 255, 255, 0);

#define WIFI_FILE "/wifi.txt"


// ============================================================================
// Owned state (this file only)
// ============================================================================

static WebServer server(80);
static DNSServer dnsServer;

static bool serverStarted = false;
static bool apMode        = false;

static String savedSSID;
static String savedPASS;


// ============================================================================
// Public helpers
// ============================================================================

const char* wifi_ap_name()
{
  return AP_SSID;
}


// ============================================================================
// Credential storage
// ============================================================================

static bool loadCreds()
{
  if (!LittleFS.exists(WIFI_FILE)) {
    LOG_WIFI("Creds", "file missing");
    return false;
  }

  File f = LittleFS.open(WIFI_FILE, "r");
  if (!f) {
    LOG_WIFI("Creds", "open failed");
    return false;
  }

  savedSSID = f.readStringUntil('\n');
  savedPASS = f.readStringUntil('\n');

  f.close();

  savedSSID.trim();
  savedPASS.trim();

  if (savedSSID.isEmpty()) {
    LOG_WIFI("Creds", "empty");
    return false;
  }

  LOG_WIFI("Creds", "loaded");
  return true;
}

static void saveCreds(const String& ssid, const String& pass)
{
  File f = LittleFS.open(WIFI_FILE, "w");
  if (!f) {
    LOG_ERROR("WiFi", "save failed");
    return;
  }

  f.println(ssid);
  f.println(pass);
  f.close();

  LOG_WIFI("Creds", "saved");
}

void wifi_set_credentials(const String& ssid, const String& pass)
{
  saveCreds(ssid, pass);
}


// ============================================================================
// Boot-time mode decision
//
// Decides the initial SYSTEM MODE based on stored Wi-Fi credentials.
//
// IMPORTANT:
//  - This function does NOT start Wi-Fi directly
//  - setMode() transfers control to system_mode.cpp
//  - Wi-Fi startup (STA/AP) happens in ENTER actions there
// ============================================================================

void initWifi()
{
  LOG_WIFI("Init", "starting");

  if (loadCreds()) {
    LOG_WIFI("Mode", "HOME");
    setMode(MODE_HOME);
  }
  else {
    LOG_WIFI("Mode", "FIELD_CONFIG");
    setMode(MODE_FIELD_CONFIG);
  }
}


// ============================================================================
// Web server lifecycle (delegated)
// ============================================================================

static void startServer()
{
  if (serverStarted) return;

  webserver_start(server);
  serverStarted = true;
}


// ============================================================================
// Mode actions
// ============================================================================

void wifi_start_sta()
{
  LOG_WIFI("STA", "starting");

  wifi_stop();

  if (!loadCreds()) {
    LOG_WIFI("STA", "no credentials → FIELD");
    setMode(MODE_FIELD_CONFIG);
    return;
  }

  apMode = false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPASS.c_str());

  LOG_WIFI("STA", "connecting");

  const unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 3000) {
    delay(50);
  }

  if (WiFi.status() == WL_CONNECTED) {
    LOG_WIFI("IP", "%s", WiFi.localIP().toString().c_str());

    if (!MDNS.begin(HOSTNAME)) {
      LOG_WIFI("mDNS", "failed");
    } else {
      LOG_WIFI("mDNS", "started");
    }

    startServer();
  }
  else {
    LOG_WIFI("STA", "failed → FIELD_CONFIG");
    setMode(MODE_FIELD_CONFIG);
  }
}


void wifi_start_ap()
{
  wifi_stop();

  apMode = true;

  LOG_WIFI("MODE", "FIELD / AP");
  LOG_WIFI("AP", "starting (%s)", AP_SSID);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMask);
  WiFi.softAP(AP_SSID, AP_PASS);

  // Wait briefly for AP IP to become valid
  IPAddress ip;
  const unsigned long t0 = millis();
  do {
    ip = WiFi.softAPIP();
    delay(10);
  } while (ip == IPAddress(0, 0, 0, 0) && millis() - t0 < 500);

  LOG_WIFI("AP IP", "%s (%s)", ip.toString().c_str(), AP_SSID);

  dnsServer.start(53, "*", apIP);
  LOG_WIFI("DNS", "captive portal");

  LOG_WIFI("LOGIN", "Phone WiFi → %s", AP_SSID);
  LOG_WIFI("LOGIN", "Browser → http://192.168.4.1");

  startServer();
}


void wifi_stop()
{
  if (!serverStarted && WiFi.getMode() == WIFI_OFF) return;

  LOG_WIFI("Stop", "WiFi");

  webserver_stop();
  server.stop();

  WiFi.disconnect(true, true);
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

  if (apMode) {
    dnsServer.processNextRequest();
  }
}


// ============================================================================
// Factory reset
// ============================================================================

void wifi_factory_reset()
{
  if (LittleFS.exists(WIFI_FILE)) {
    LittleFS.remove(WIFI_FILE);
    LOG_WIFI("Reset", "wifi.txt removed");
  }
}


