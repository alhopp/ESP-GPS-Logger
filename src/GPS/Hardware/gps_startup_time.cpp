#include "GPS/Hardware/gps_startup_time.h"

// Sends RTC-cached time to the GPS as a warm-start hint.
// System time is still set later from real NAV-PVT GPS time.

#include <Arduino.h>
#include <string.h>

#include "Core/log.h"
#include "Core/Rtc/rtc_time_state.h"
#include "GPS/Ublox/ublox_driver.h"

namespace {
void ubxSend(const uint8_t* msg, size_t len)
{
  UbloxSerial.write(msg, len);
  UbloxSerial.flush();
}
}

void gps_send_time_from_rtc()
{
  if (RTC_year < 2020 || RTC_month == 0 || RTC_day == 0) {
    LOG_GPS("Time", "RTC invalid, skip");
    return;
  }

  LOG_GPS("Time", "inject %04d-%02d-%02d %02d:%02d",
          RTC_year, RTC_month, RTC_day, RTC_hour, RTC_min);

  uint8_t payload[20] = {0};

  // iTOW = 0, tAcc ~= 5000 ns.
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

  uint8_t ckA = 0;
  uint8_t ckB = 0;
  for (int i = 2; i < 6 + sizeof(payload); i++) {
    ckA += msg[i];
    ckB += ckA;
  }

  msg[26] = ckA;
  msg[27] = ckB;

  ubxSend(msg, sizeof(msg));
}
