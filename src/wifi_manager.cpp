#include "wifi_manager.h"

#include <WiFi.h>
#include "ESP_functions.h"   // config, globals (Wifi_on, SoftAP_connection, etc)

// -----------------------------------------------------------------------------
// Module-local state (ONLY wifi_manager owns this)
// -----------------------------------------------------------------------------

bool ap_mode = false;
bool wifi_is_connected();


static const char* ssid_primary   = nullptr;
static const char* pass_primary   = nullptr;
static const char* ssid_secondary = nullptr;
static const char* pass_secondary = nullptr;

static constexpr const char* AP_SSID = "ESP32AP";
static constexpr const char* AP_PASS = "password";

// -----------------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------------

static void bindConfig()
{
  ssid_primary   = config.ssid;
  pass_primary   = config.password;
  ssid_secondary = config.ssid2;
  pass_secondary = config.password2;
}

static void startSTA(const char* ssid, const char* pass)
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  actual_ssid = ssid;
  wifi_search = 10;

  Serial.print("Searching for ");
  Serial.println(ssid);
  Search_for_wifi();
}

static void startAP()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  wifi_search = 120;
  Serial.println("Starting Access Point");
  Search_for_wifi();
}

static void handleConnected()
{
  actual_ssid = WiFi.SSID();

  Serial.println();
  Serial.print("Connected to ");
  Serial.println(actual_ssid);
  Serial.print("IP address: ");
  Serial.println(IP_adress);

  Wifi_on = true;
  ftpSrv.begin("esp32", "esp32");
  // OTA_setup();   // optional
}

static void handleDisconnected()
{
  detachInterrupt(GO_TO_SLEEP_GPIO);

  Serial.println("No WiFi connection");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  Wifi_on = false;
  SoftAP_connection = false;

  Update_screen(GPS_INIT_SCREEN);
  GPS_OK = setupGPS();
  Update_screen(GPS_INIT_SCREEN);
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void wifi_init()
{
  bindConfig();

  WiFi.onEvent(OnWiFiEvent);

  Serial.print("T5 MAC address: ");
  WiFi.macAddress(mac);

  // --- Primary STA attempt ---
  startSTA(ssid_primary, pass_primary);

  // --- Secondary STA fallback ---
  if (WiFi.status() != WL_CONNECTED && !ap_mode) {
    startSTA(ssid_secondary, pass_secondary);
  }

  // --- AP mode fallback ---
  if (ap_mode) {
    startAP();
  }

  // --- Final state ---
  if (WiFi.status() == WL_CONNECTED || SoftAP_connection) {
    handleConnected();
  } else {
    handleDisconnected();
  }
}

void wifi_handle()
{
  if (wifi_is_connected()) {
    ftpSrv.handleFTP();
    // server.handleClient();  // optional
  }
}

bool wifi_is_connected()
{
  return (WiFi.status() == WL_CONNECTED) || SoftAP_connection;
}
