// ---------------------------------------------------------------------------
// Wi-Fi mode selection:
// - Loads saved Wi-Fi credentials (if present)
// - Selects initial system mode (HOME or FIELD_CONFIG)
//
// Does NOT start Wi-Fi or networking yet.
// Actual Wi-Fi setup is handled later by the mode manager.
// ---------------------------------------------------------------------------

#include "wifi_manager.h"
#include "system_mode.h"

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <LittleFS.h>

// Logging macros
#include "Definitions.h"

// ==================================================
// CONFIG
// ==================================================
static const char* HOSTNAME = "esp32";

// AP (field mode)
static const char* AP_SSID = "ESP32 GPS";
static const char* AP_PASS = "12345678";   // iOS requires 8+

static IPAddress apIP(192, 168, 4, 1);
static IPAddress netMask(255, 255, 255, 0);

#define WIFI_FILE "/wifi.txt"

// ==================================================
// OWNED GLOBALS (this file only)
// ==================================================
static WebServer server(80);
static DNSServer dnsServer;

static bool serverStarted = false;
static bool apMode = false;

static String savedSSID;
static String savedPASS;

// ==================================================
// WIFI SETUP PAGE
// ==================================================
static const char* wifiForm = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 GPS Wi-Fi</title>
<style>
 body { font-family: monospace; text-align: center; }
 input { font-size: 18px; padding: 8px; width: 90%; max-width: 300px; }
 button { font-size: 20px; padding: 10px 30px; }
</style>
</head>
<body>
<h2>ESP32 GPS</h2>
<h3>Wi-Fi Setup</h3>
<form action="/save" method="POST">
  <p><input name="ssid" placeholder="Wi-Fi SSID" required></p>
  <p><input name="pass" type="password" placeholder="Wi-Fi Password"></p>
  <p><button type="submit">Save & Connect</button></p>
</form>
</body>
</html>
)rawliteral";

// ==================================================
// CREDENTIAL STORAGE (FS already mounted elsewhere)
// ==================================================
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

  savedSSID.trim();
  savedPASS.trim();
  f.close();

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

// ==================================================
// INIT (MODE DECISION ONLY)
// ==================================================
void initWifi()
{
  LOG_WIFI("Init", "starting");

  if (loadCreds()) {
    LOG_WIFI("Mode", "HOME");
    setMode(MODE_HOME);
  } else {
    LOG_WIFI("Mode", "FIELD_CONFIG");
    setMode(MODE_FIELD_CONFIG);
  }
}

// ==================================================
// INTERNAL HELPERS
// ==================================================
static void startServer()
{
  if (serverStarted) return;

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", wifiForm);
  });

  server.on("/save", HTTP_POST, []() {
    const String ssid = server.arg("ssid");
    const String pass = server.arg("pass");

    saveCreds(ssid, pass);

    server.send(
      200,
      "text/html",
      "<h3>Saved. Connecting…</h3><p>You may close this page.</p>"
    );

    delay(300);
    setMode(MODE_HOME);
  });

  server.onNotFound([]() {
    server.send(200, "text/html", wifiForm);
  });

  server.begin();
  serverStarted = true;

  LOG_WIFI("Web", "server started");
}

// ==================================================
// MODE ENTRY / EXIT ACTIONS
// ==================================================
void wifi_start_sta()
{
  LOG_WIFI("STA", "starting");

  wifi_stop();

  if (!loadCreds()) {
    LOG_WIFI("STA", "no credentials");
    return;
  }

  apMode = false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPASS.c_str());

  LOG_WIFI("STA", "connecting");

  if (!MDNS.begin(HOSTNAME)) {
    LOG_WIFI("mDNS", "failed");
  } else {
    LOG_WIFI("mDNS", "started");
  }

  // Wait briefly for IP (non-blocking friendly)
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 3000) {
    delay(50);
  }

  if (WiFi.status() == WL_CONNECTED) {
    LOG_WIFI(
      "IP",
      "%s",
      WiFi.localIP().toString().c_str()
    );
  } else {
    LOG_WIFI("IP", "not assigned");
  }

  startServer();
}


void wifi_start_ap()
{
  LOG_WIFI("AP", "starting");

  wifi_stop();

  apMode = true;

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMask);
  WiFi.softAP(AP_SSID, AP_PASS);

  LOG_WIFI("AP IP", "%s", WiFi.softAPIP().toString().c_str());

  dnsServer.start(53, "*", apIP);
  LOG_WIFI("DNS", "captive portal");

  startServer();
}


void wifi_stop()
{
  if (!serverStarted && WiFi.getMode() == WIFI_OFF) return;

  LOG_WIFI("Stop", "WiFi");

  server.stop();
  dnsServer.stop();

  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);

  serverStarted = false;
  apMode = false;
}

// ==================================================
// LOOP SERVICE
// ==================================================
void wifi_loop()
{
  if (!serverStarted) return;

  server.handleClient();

  if (apMode) {
    dnsServer.processNextRequest();
  }
}
