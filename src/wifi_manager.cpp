#include "wifi_manager.h"

#include <Arduino.h>
#include <WiFi.h>

#include "ESP_functions.h"   // config, mac, ftpSrv (optional)
#include "config_manager.h"

// ----------------------------------------------------------------------------
// Simple STA-only WiFi
// ----------------------------------------------------------------------------

static bool wifi_started = false;

void wifi_init()
{
  const char* ssid = config.ssid;
  const char* pass = config.password;

  Serial.print("[WIFI] MAC: ");
  WiFi.macAddress(mac);

  // If no SSID configured → do nothing, WiFi disabled
  if (!ssid || !strlen(ssid) || strcmp(ssid, "ssid_not_set") == 0) {
    Serial.println("[WIFI] ssid_not_set → WiFi disabled");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Wifi_on = false;
    return;
  }

  Serial.print("[WIFI] STA begin: ");
  Serial.println(ssid);

  WiFi.disconnect(true);
  delay(50);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  wifi_started = true;
  Wifi_on = false;
}

void wifi_handle()
{
  if (!wifi_started) return;

  if (WiFi.status() == WL_CONNECTED) {
    if (!Wifi_on) {
      Wifi_on = true;
      Serial.print("[WIFI] Connected, IP=");
      Serial.println(WiFi.localIP());

      // Optional: FTP / OTA only once connected
      // ftpSrv.begin("esp32", "esp32");
    }

    // Optional background services
    // ftpSrv.handleFTP();
  }
}

bool wifi_is_connected()
{
  return (WiFi.status() == WL_CONNECTED);
}
