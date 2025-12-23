#include "wifi_manager.h"
#include "system_mode.h"

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <LittleFS.h>

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
  if (!LittleFS.exists(WIFI_FILE)) return false;

  File f = LittleFS.open(WIFI_FILE, "r");
  if (!f) return false;

  savedSSID = f.readStringUntil('\n');
  savedPASS = f.readStringUntil('\n');

  savedSSID.trim();
  savedPASS.trim();

  f.close();

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

// ==================================================
// INIT (MODE DECISION ONLY)
// ==================================================
void initWifi()
{
  Serial.println("[WiFi   ] Initialising");

  if (loadCreds()) {
    Serial.println("[WiFi   ] Credentials found → MODE_HOME");
    setMode(MODE_HOME);
  } else {
    Serial.println("[WiFi   ] No credentials → MODE_FIELD_CONFIG");
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

  Serial.println("[WiFi   ] Web server started");
}

// ==================================================
// MODE ENTRY / EXIT ACTIONS
// ==================================================
void wifi_start_sta()
{
  Serial.println("[WiFi   ] Starting STA mode");

  wifi_stop();

  if (!loadCreds()) {
    Serial.println("[WiFi   ] No saved credentials");
    return;
  }

  apMode = false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPASS.c_str());

  if (!MDNS.begin(HOSTNAME)) {
    Serial.println("[WiFi   ] mDNS failed");
  }

  startServer();
}

void wifi_start_ap()
{
  Serial.println("[WiFi   ] Starting AP mode");

  wifi_stop();

  apMode = true;

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMask);
  WiFi.softAP(AP_SSID, AP_PASS);

  dnsServer.start(53, "*", apIP);

  startServer();
}

void wifi_stop()
{
  if (!serverStarted && WiFi.getMode() == WIFI_OFF) return;

  Serial.println("[WiFi   ] Stopping WiFi");

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
