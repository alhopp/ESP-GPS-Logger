#include "wifi_manager.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

#include "ESP_functions.h"
#include "config_manager.h"
#include "E_paper.h"

#include <Fonts/FreeSansBold9pt7b.h>
#include <QRCode.h>

// -----------------------------------------------------------------------------
// RP6 Hall switch
// GPIO12 LOW = setup / network mode
// -----------------------------------------------------------------------------
#define WIFI_MODE_PIN 12

// -----------------------------------------------------------------------------
// Captive portal
// -----------------------------------------------------------------------------
static DNSServer dnsServer;
static WebServer server(80);
static const byte DNS_PORT = 53;
static IPAddress apIP(192, 168, 4, 1);

// -----------------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------------
static bool wifi_started = false;

// -----------------------------------------------------------------------------
// QR helper
// -----------------------------------------------------------------------------
static void drawQRCode(const char* text, int x0, int y0, int scale)
{
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];

  qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, text);

  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        display.fillRect(
          x0 + x * scale,
          y0 + y * scale,
          scale,
          scale,
          GxEPD_BLACK
        );
      }
    }
  }
}

// -----------------------------------------------------------------------------
// SETUP DISPLAY (QR + text only)
// -----------------------------------------------------------------------------
static void displaySetup()
{
  // Force full clear to kill ghosting
  display.setFullWindow();

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);

    // QR code (moved UP)
    drawQRCode(
      "http://captive.apple.com",
      85,   // X (centered)
      5,   // Y (higher)
      3
    );

    // Instruction text
    display.setFont(&FreeSansBold9pt7b);

    display.setCursor(28, 118);
    display.print("Join ESP and then Scan QR");

  } while (display.nextPage());
}

// -----------------------------------------------------------------------------
// HTML UI
// -----------------------------------------------------------------------------
static const char WIFI_SETUP_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP GPS Setup</title>
<style>
body {
  font-family: -apple-system, BlinkMacSystemFont, sans-serif;
  background:#f4f4f4;
  margin:0; padding:20px;
}
.card {
  background:#fff;
  max-width:360px;
  margin:auto;
  padding:20px;
  border-radius:10px;
  box-shadow:0 2px 8px rgba(0,0,0,.1);
}
h1 { text-align:center; }
label { font-size:14px; margin-top:12px; display:block; }
input {
  width:100%; padding:10px; font-size:16px;
  margin-top:6px; border-radius:6px; border:1px solid #ccc;
}
button {
  margin-top:20px; width:100%;
  padding:12px; font-size:16px;
  border:none; border-radius:8px;
  background:#007aff; color:white;
}
</style>
</head>
<body>
<div class="card">
<h1>ESP GPS Setup</h1>
<form action="/save" method="POST">
<label>WiFi SSID</label>
<input name="ssid" required>
<label>Password</label>
<input name="pass" type="password">
<button type="submit">Save & Reboot</button>
</form>
</div>
</body>
</html>
)rawliteral";

// -----------------------------------------------------------------------------
// Web handlers
// -----------------------------------------------------------------------------
static void handleRoot()
{
  server.send_P(200, "text/html", WIFI_SETUP_PAGE);
}

static void handleAppleProbe()
{
  // Serve the real setup page so iOS opens the captive UI
  server.send_P(200, "text/html", WIFI_SETUP_PAGE);
}


static void handleNotFound()
{
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// -----------------------------------------------------------------------------
// Save WiFi config
// -----------------------------------------------------------------------------
static void handleSave()
{
  if (!server.hasArg("ssid")) {
    server.send(400, "text/plain", "Missing SSID");
    return;
  }

  strncpy(config.ssid, server.arg("ssid").c_str(), sizeof(config.ssid) - 1);
  strncpy(config.password, server.arg("pass").c_str(), sizeof(config.password) - 1);

  Serial.println(F("[WIFI  ] Saving WiFi config"));
  saveConfig();

  server.send(200, "text/html",
    "<html><body><h2>Saved</h2><p>Rebooting…</p></body></html>");

  delay(1000);
  ESP.restart();
}

// -----------------------------------------------------------------------------
// Start AP + captive portal
// -----------------------------------------------------------------------------
static void startCaptivePortal()
{
  WiFi.disconnect(true);
  delay(50);

  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP-GPS-SETUP");
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));

  Serial.print(F("[WIFI  ] AP ACTIVE, IP="));
  Serial.println(WiFi.softAPIP());

  displaySetup();

  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);

  // captive probes → SUCCESS ONLY
  server.on("/hotspot-detect.html", handleAppleProbe);

  // everything else → redirect to /
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println(F("[WIFI  ] Captive portal started"));
}

// -----------------------------------------------------------------------------
// Start STA
// -----------------------------------------------------------------------------
static void startSTA(const char* ssid, const char* pass)
{
  WiFi.disconnect(true);
  delay(50);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  Serial.print(F("[WIFI  ] STA begin: "));
  Serial.println(ssid);

  wifi_started = true;
  Wifi_on = false;
}

// -----------------------------------------------------------------------------
// WiFi init
// -----------------------------------------------------------------------------
void wifi_init()
{
  pinMode(WIFI_MODE_PIN, INPUT);
  delay(50);

  bool hallActive = (digitalRead(WIFI_MODE_PIN) == LOW);
  bool ssidValid  = strlen(config.ssid) && strcmp(config.ssid, "ssid_not_set") != 0;

  if (!hallActive) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    wifi_started = false;
    Wifi_on = false;
    Serial.println(F("[WIFI  ] GPS mode (WiFi off)"));
    return;
  }

  if (ssidValid) {
    displaySetup();
    startSTA(config.ssid, config.password);
    return;
  }

  Serial.println(F("[WIFI  ] Setup mode (QR)"));
  startCaptivePortal();
}

// -----------------------------------------------------------------------------
// Background handler
// -----------------------------------------------------------------------------
void wifi_handle()
{
  if (WiFi.getMode() == WIFI_AP) {
    dnsServer.processNextRequest();
    server.handleClient();
    return;
  }

  if (wifi_started && WiFi.status() == WL_CONNECTED && !Wifi_on) {
    Wifi_on = true;
    Serial.print(F("[WIFI  ] Connected, IP="));
    Serial.println(WiFi.localIP());
  }
}

// -----------------------------------------------------------------------------
bool wifi_is_connected()
{
  return WiFi.status() == WL_CONNECTED;
}
