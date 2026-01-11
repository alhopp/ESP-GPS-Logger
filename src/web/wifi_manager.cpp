// ============================================================================
// Wi-Fi Manager
//  - STA if creds exist
//  - AP captive portal if not
//  - Wi-Fi scan ONCE when AP starts (AP mode only)
// ============================================================================

#include <ArduinoJson.h>
#include "web/wifi_manager.h"
#include "system_mode.h"
#include "task_display.h"
#include "Layout.h"

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

static IPAddress apIP(192, 168, 4, 1);
static IPAddress netMask(255, 255, 255, 0);

#define WIFI_FILE "/wifi.txt"

// ============================================================================
// Forward decls
// ============================================================================
static void startServer();
static bool loadCreds();
static void saveCreds(const String& ssid, const String& pass);
//static bool ensureLittleFSMounted();

// ============================================================================
// State
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

bool wifi_is_ap_mode()
{
  return apMode;
}


// ============================================================================
// Credential storage
// ============================================================================

static bool loadCreds()
{
 // if (!ensureLittleFSMounted()) return false;

  if (!LittleFS.exists(WIFI_FILE)) return false;

  File f = LittleFS.open(WIFI_FILE, "r");
  if (!f) {
    LOG_WIFI("FS", "Open read failed: %s", WIFI_FILE);
    return false;
  }

  savedSSID = f.readStringUntil('\n');
  savedPASS = f.readStringUntil('\n');
  f.close();

  savedSSID.trim();
  savedPASS.trim();

  if (savedSSID.isEmpty()) {
    LOG_WIFI("CREDS", "SSID empty");
    return false;
  }

  return true;
}

static void saveCreds(const String& ssid, const String& pass)
{
  //if (!ensureLittleFSMounted()) return;

  File f = LittleFS.open(WIFI_FILE, "w");
  if (!f) {
    LOG_WIFI("FS", "Open write failed: %s", WIFI_FILE);
    return;
  }

  f.println(ssid);
  f.println(pass);
  f.close();

  // Keep cached values in sync (useful if you switch to STA immediately)
  savedSSID = ssid;
  savedPASS = pass;

  LOG_WIFI("CREDS", "Saved ssid='%s' pass_len=%d", ssid.c_str(), pass.length());
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
   
  loadCreds();             
    setMode(MODE_WIFI_SOFT_AP);

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
// STA mode - enable internet uplink
// ============================================================================

void wifi_start_sta()
{
  if (!loadCreds()) {
    LOG_WIFI("STA", "No creds → FIELD_CFG");
    setMode(MODE_WIFI_SOFT_AP);
    return;
  }

  apMode   = true;

  LOG_WIFI("STA", "Connecting to '%s' pass_len=%d",
           savedSSID.c_str(), savedPASS.length());

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(savedSSID.c_str(), savedPASS.c_str());

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < 15000UL) {
    delay(100);
  }

  if (WiFi.status() != WL_CONNECTED) {
  LOG_WIFI("STA", "Failed → AP only");
  return;
}

LOG_WIFI("STA", "Connected IP=%s", WiFi.localIP().toString().c_str());

if (MDNS.begin(HOSTNAME)) {
  MDNS.addService("http", "tcp", 80);
}


}


// = device control via phone

void wifi_start_ap()
{
  apMode   = true;

  WiFi.mode(WIFI_AP_STA);   // AP always allowed, STA preserved if present
  WiFi.softAPConfig(apIP, apIP, netMask);
  WiFi.softAP(AP_SSID, AP_PASS);

  unsigned long t0 = millis();
  while (WiFi.softAPIP() == IPAddress(0, 0, 0, 0) && (millis() - t0) < 500UL) {
    delay(10);
  }

  dnsServer.start(53, "*", WiFi.softAPIP());

  startServer();

  LOG_WIFI("AP", "Started SSID='%s' IP=%s",
           AP_SSID, WiFi.softAPIP().toString().c_str());
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

  // full disconnect + erase old state
  WiFi.disconnect(true);
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
 // if (!ensureLittleFSMounted()) return;

  if (LittleFS.exists(WIFI_FILE)) {
    LittleFS.remove(WIFI_FILE);
    LOG_WIFI("CREDS", "Factory reset: removed %s", WIFI_FILE);
  }
}

bool wifi_has_credentials()
{
  return !savedSSID.isEmpty();
}

String wifi_get_saved_ssid()
{
  return savedSSID;
}

String wifi_get_saved_pass()
{
  return savedPASS;
}

