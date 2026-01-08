// ============================================================================
// gps_manager.cpp
//
// SAFE GPS bring-up for ESP32 + u-blox
// - RTC-cached baud (warm boot)
// - Full scan fallback
// - UBX-MON-VER probe
// - RTC time injection (UBX-CFG-TIMEUTC)
// ============================================================================

#include "gps_manager.h"

#include <Arduino.h>
#include <HardwareSerial.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>

#include "Definitions.h"
#include "rtc_state.h"     // RTC_gps_* + RTC time fields
#include "Ublox.h"         // ubx::poll::mon_ver definition

// -----------------------------------------------------------------------------
// BAUD TABLE
// -----------------------------------------------------------------------------
static const uint32_t gpsBauds[] = {
  0,        // index 0 = invalid
  9600,     // index 1
  38400,    // index 2
  115200    // index 3
};

// -----------------------------------------------------------------------------
// POWER CONTROL
// -----------------------------------------------------------------------------
void gps_power_on()
{
  pinMode(UBLOX_POWER1, OUTPUT);
  pinMode(UBLOX_POWER2, OUTPUT);
  pinMode(UBLOX_POWER3, OUTPUT);

  rtc_gpio_set_drive_capability(
    static_cast<gpio_num_t>(UBLOX_RTC_GPIO1),
    GPIO_DRIVE_CAP_3
  );

  rtc_gpio_set_drive_capability(
    static_cast<gpio_num_t>(UBLOX_RTC_GPIO2),
    GPIO_DRIVE_CAP_3
  );

  gpio_set_drive_capability(
    static_cast<gpio_num_t>(UBLOX_GPIO3),
    GPIO_DRIVE_CAP_3
  );


  delay(50);

  digitalWrite(UBLOX_POWER1, HIGH);
  digitalWrite(UBLOX_POWER2, HIGH);
  digitalWrite(UBLOX_POWER3, HIGH);

  delay(150);
}

static void gps_power_off_impl()
{
  digitalWrite(UBLOX_POWER1, LOW);
  digitalWrite(UBLOX_POWER2, LOW);
  digitalWrite(UBLOX_POWER3, LOW);
}

void gps_power_off()
{
  gps_power_off_impl();
}

// -----------------------------------------------------------------------------
// UBX SEND HELPER
// -----------------------------------------------------------------------------
static void ubxSend(const uint8_t* msg, size_t len)
{
  UbloxSerial.write(msg, len);
  UbloxSerial.flush();
}

// -----------------------------------------------------------------------------
// RTC TIME INJECTION (UBX-CFG-TIMEUTC)
// -----------------------------------------------------------------------------
static void gps_send_time_from_rtc()
{
  if (RTC_year < 2020 || RTC_month == 0 || RTC_day == 0) {
    LOG_GPS("Time", "RTC invalid, skip");
    return;
  }

  LOG_GPS("Time", "inject %04d-%02d-%02d %02d:%02d",
          RTC_year, RTC_month, RTC_day, RTC_hour, RTC_min);

  uint8_t payload[20] = {0};

  // iTOW = 0
  // tAcc ≈ 5000 ns
  payload[4] = 0x88;
  payload[5] = 0x13;

  payload[12] = RTC_year & 0xFF;
  payload[13] = RTC_year >> 8;
  payload[14] = RTC_month;
  payload[15] = RTC_day;
  payload[16] = RTC_hour;
  payload[17] = RTC_min;
  payload[18] = 0;      // seconds unknown
  payload[19] = 0x07;   // valid date + time + resolved

  uint8_t msg[28];
  msg[0] = 0xB5;
  msg[1] = 0x62;
  msg[2] = 0x06;
  msg[3] = 0x5C;
  msg[4] = sizeof(payload);
  msg[5] = 0x00;

  memcpy(&msg[6], payload, sizeof(payload));

  uint8_t ckA = 0, ckB = 0;
  for (int i = 2; i < 6 + sizeof(payload); i++) {
    ckA += msg[i];
    ckB += ckA;
  }

  msg[26] = ckA;
  msg[27] = ckB;

  ubxSend(msg, sizeof(msg));
}

// -----------------------------------------------------------------------------
// PROBE HELPER (MON-VER)
// -----------------------------------------------------------------------------
static bool probe_gps(uint32_t baud)
{
  UbloxSerial.begin(baud, SERIAL_8N1, GPS_UART_RX_PIN, GPS_UART_TX_PIN);
  delay(120);

  // DO NOT DRAIN RX BUFFER

  // Send MON-VER poll
  UbloxSerial.write(
    ubx::poll::mon_ver,
    sizeof(ubx::poll::mon_ver)
  );
  UbloxSerial.flush();   // TX only (safe)

  uint32_t start = millis();
  while (millis() - start < 300) {
    if (UbloxSerial.available() > 0) {
      // Bytes exist — let processGPS() consume them later
      return true;
    }
    delay(1);
  }

  return false;
}


// -----------------------------------------------------------------------------
// PUBLIC API
// -----------------------------------------------------------------------------
// initGPS()
// - Powers GPS
// - Detects current baud (RTC fast path + scan fallback)
// - Applies full M10 UBX configuration ONCE
// - Forces final baud to 38400
// - Restarts UART at 38400
// - Injects RTC time (if valid)
// - Returns GPS READY
// -----------------------------------------------------------------------------
bool initGPS()
{
  LOG_GPS("Init", "starting");

  gps_power_on();
  delay(100);

  // ---------------------------------------------------------------------------
  // Detect current baud (RTC fast path, then scan)
  // ---------------------------------------------------------------------------
  uint32_t detectedBaud = 0;

  if (RTC_gps_valid &&
      RTC_gps_baud_index >= 1 &&
      RTC_gps_baud_index <= 3) {

    uint32_t baud = gpsBauds[RTC_gps_baud_index];
    LOG_GPS("Detect", "cached baud=%lu", (unsigned long)baud);

    if (probe_gps(baud)) {
      detectedBaud = baud;
      LOG_GPS("Detect", "cache hit");
    } else {
      LOG_GPS("Detect", "cache failed → scan");
      RTC_gps_valid = false;
    }
  }

  if (detectedBaud == 0) {
    LOG_GPS("Detect", "baud scan");

    for (uint8_t i = 1; i <= 3; i++) {
      if (probe_gps(gpsBauds[i])) {
        detectedBaud = gpsBauds[i];
        RTC_gps_baud_index = i;
        RTC_gps_valid = true;
        LOG_GPS("Detect", "found baud=%lu", (unsigned long)detectedBaud);
        break;
      }
    }
  }

  if (detectedBaud == 0) {
    RTC_gps_valid = false;
    LOG_GPS("Init", "no GPS detected");
    return false;
  }

  // ---------------------------------------------------------------------------
  // Apply M10 configuration (UBX-only, GNSS, messages, SEA model)
  //     UART is currently at detectedBaud
  // ---------------------------------------------------------------------------
  LOG_GPS("Config", "apply M10 profile");
  Init_ubloxM10();   // sends UBX + switches GPS internally to 38400

  // ---------------------------------------------------------------------------
  // Force UART to final authoritative baud (38400)
  // ---------------------------------------------------------------------------
  UbloxSerial.end();
  delay(50);
  UbloxSerial.begin(38400, SERIAL_8N1, GPS_UART_RX_PIN, GPS_UART_TX_PIN);
  delay(100);

  sendUbx(ubx::msg::nav_pvt);
  sendUbx(ubx::msg::nav_dop);

  RTC_gps_baud_index = 2; // index for 38400
  RTC_gps_valid      = true;

  LOG_GPS("Detect", "locked @38400");

  // ---------------------------------------------------------------------------
  // Inject RTC time (optional, accelerates first fix)
  // ---------------------------------------------------------------------------
  gps_send_time_from_rtc();

  LOG_GPS("Init", "GPS ready");

  delay(50);

  return true;
}


GPS_data Ublox; // create an object storing GPS_data !
GPS_SAT_info Ublox_Sat;//create an object storing GPS_SAT info !
GPS_speed M100(100);
GPS_speed M250(250);
GPS_speed M500(500);
GPS_speed M1852(1852);
GPS_time S2(2);
GPS_time s2(2);
GPS_time S10(10);
GPS_time s10(10);//for  stats GPIO_12 screens, reset possible !!
GPS_time S1800(1800);
GPS_time S3600(3600);
Alfa_speed A250(50);
Alfa_speed A500(50);
Alfa_speed a500(50);//for  Alfa stats GPIO_12 screens, reset possible !!
GPS_Track M_500;