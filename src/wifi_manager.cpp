#include "wifi_manager.h"

#include <WiFi.h>
#include <WiFiUdp.h>

#include "ESP_functions.h"
#include "OTA_server.h"
#include "ESP32FtpServerJH.h"
#include "Definitions.h"
#include "GPS_data.h"
#include "E_paper.h"

// externs already exist in your project
extern const char* ssid;
extern const char* password;
extern const char* ssid2;
extern const char* password2;
extern const char* soft_ap_ssid;
extern const char* soft_ap_password;

void wifi_init() {

  WiFi.onEvent(OnWiFiEvent);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("T5 MAC adress: ");
  WiFi.macAddress(mac);
  actual_ssid = ssid;

  Serial.println("search SSID1");
  Search_for_wifi();

  if ((WiFi.status() != WL_CONNECTED) && (ap_mode == false)) {
    WiFi.disconnect();
    WiFi.begin(ssid2, password2);
    actual_ssid = ssid2;
    wifi_search = 10;
  }

  if (ap_mode == false) {
    Serial.println("search SSID2");
    Search_for_wifi();
  }

  if (ap_mode == true) {
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAP(soft_ap_ssid, soft_ap_password);
    wifi_search = 120;
    Serial.println("search AP");
    Search_for_wifi();
  }

  if ((WiFi.status() == WL_CONNECTED) || SoftAP_connection) {
    actual_ssid = WiFi.SSID();

    Serial.println();
    Serial.print("Connected to ");
    Serial.println(actual_ssid);
    Serial.print("IP address: ");
    Serial.println(IP_adress);

    Wifi_on = true;
    ftpSrv.begin("esp32", "esp32");
    OTA_setup();
  } else {
    detachInterrupt(GO_TO_SLEEP_GPIO);

    Serial.println("No Wifi connection !");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    Wifi_on = false;
    SoftAP_connection = false;

    Update_screen(GPS_INIT_SCREEN);
    GPS_OK = setupGPS();
    Update_screen(GPS_INIT_SCREEN);
  }
}

void wifi_handle() {
  if ((WiFi.status() == WL_CONNECTED) || SoftAP_connection) {
    ftpSrv.handleFTP();
    server.handleClient();
  }
}

bool wifi_is_connected() {
  return (WiFi.status() == WL_CONNECTED) || SoftAP_connection;
}
