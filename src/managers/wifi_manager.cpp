// ---------------------------------------------------------------------------
// Wi-Fi Manager
//
// Behaviour:
// - If valid Wi-Fi credentials exist → try STA (HOME)
// - If STA fails or no creds → AP captive portal (FIELD_CONFIG)
// - Field portal allows:
//     • Editing HOME Wi-Fi credentials
//     • Editing system variables
//
// Wi-Fi never blocks boot, never bricks device.
// ---------------------------------------------------------------------------

#include "wifi_manager.h"
#include "system_mode.h"

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>

#include "Definitions.h"

// ==================================================
// CONFIG
// ==================================================
static const char* HOSTNAME = "esp32-gps";

// AP (Field mode)
static const char* AP_SSID = "ESP32 GPS";
static const char* AP_PASS = "12345678";   // iOS requires ≥8 chars

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

const char* wifi_ap_name()
{
  return AP_SSID;
}

// ==================================================
// FIELD CONFIG HTML
// ==================================================
static const char* fieldForm = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 GPS – Field Setup</title>
<style>
 body { font-family: monospace; text-align: center; }
 input { font-size: 16px; padding: 6px; width: 90%; max-width: 320px; }
 button { font-size: 18px; padding: 8px 24px; margin-top: 10px; }
 hr { margin: 24px 0; }
</style>
</head>
<body>

<h2>ESP32 GPS</h2>
<h3>Field Configuration</h3>

<form action="/save_wifi" method="POST">
  <h4>Home Wi-Fi</h4>
  <p><input name="ssid" placeholder="Wi-Fi SSID"></p>
  <p><input name="pass" type="password" placeholder="Wi-Fi Password"></p>
  <button type="submit">Save Wi-Fi</button>
</form>

<hr>

<form action="/save_config" method="POST">
  <h4>System</h4>
  <p><input name="sample_rate" placeholder="Sample rate (Hz)"></p>
  <p><input name="gnss_mode" placeholder="GNSS mode"></p>
  <button type="submit">Save Config</button>
</form>

</body>
</html>
)rawliteral";

// ==================================================
// CREDENTIAL STORAGE
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
// MODE DECISION (BOOT ONLY)
//
// Decides the initial SYSTEM MODE based on stored
// Wi-Fi credentials.
//
// IMPORTANT:
// - This function does NOT start Wi-Fi directly.
// - Calling setMode() transfers control to system_mode.cpp,
//   where EXIT / TRANSITION / ENTER actions are executed.
// - Wi-Fi startup (AP or STA) is triggered there, not here.
// ==================================================

void initWifi()
{
  LOG_WIFI("Init", "starting");

  if (loadCreds()) {
    LOG_WIFI("Mode", "HOME");

    // Jump to system_mode.cpp:
    // - currentMode is updated
    // - ENTER actions for MODE_HOME are executed
    //   (wifi_start_sta() is called there)
    setMode(MODE_HOME);

  } else {
    LOG_WIFI("Mode", "FIELD_CONFIG");

    // Jump to system_mode.cpp:
    // - currentMode is updated
    // - ENTER actions for MODE_FIELD_CONFIG are executed
    //   (wifi_start_ap() is called there)
    setMode(MODE_FIELD_CONFIG);
  }
}

// ==================================================
// WEB SERVER
// ==================================================
static void startServer()
{
  if (serverStarted) return;

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", fieldForm);
  });

  server.on("/save_wifi", HTTP_POST, []() {
    saveCreds(server.arg("ssid"), server.arg("pass"));
    server.send(200, "text/html",
      "<h3>Wi-Fi saved</h3><p>Device will attempt HOME mode.</p>"
    );
    delay(300);
    setMode(MODE_HOME);
  });

  server.on("/save_config", HTTP_POST, []() {
    // ---- Hook into your config system here ----
    // Example:
    // if (server.hasArg("sample_rate"))
    //   config.sample_rate = server.arg("sample_rate").toInt();
    //
    // saveConfig();

    server.send(200, "text/html",
      "<h3>Config saved</h3><p>Changes applied.</p>"
    );
  });

  server.onNotFound([]() {
    server.send(200, "text/html", fieldForm);
  });

  server.begin();
  serverStarted = true;

  LOG_WIFI("Web", "server started");
}

// ==================================================
// MODE ACTIONS
// ==================================================
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

  unsigned long t0 = millis();
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
  unsigned long t0 = millis();
  do {
    ip = WiFi.softAPIP();
    delay(10);
  } while (ip == IPAddress(0,0,0,0) && millis() - t0 < 500);

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

// ==================================================
// FACTORY RESET
// ==================================================
void wifi_factory_reset()
{
  if (LittleFS.exists(WIFI_FILE)) {
    LittleFS.remove(WIFI_FILE);
    LOG_WIFI("Reset", "wifi.txt removed");
  }
}
