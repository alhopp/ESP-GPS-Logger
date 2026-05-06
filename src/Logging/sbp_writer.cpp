#include "Logging/sbp_writer.h"
#include "GPS/Ublox/ublox_driver.h"
#include "Core/Globals.h"

namespace {
struct SBP_Header { // 64 bytes
  uint16_t Text_length;
  uint8_t  Id1;
  uint8_t  Id2;
  uint16_t Again_length;
  uint8_t  Start;
  char     Identity[57];
} __attribute__((packed));

struct SBP_frame { // 32 bytes
  uint8_t  HDOP;
  uint8_t  SVIDCnt;
  uint16_t UtcSec;
  uint32_t date_time_UTC_packed;
  uint32_t SVIDList;
  int32_t  Lat;
  int32_t  Lon;
  int32_t  AltCM;
  uint16_t Sog;
  uint16_t Cog;
  int16_t  ClmbRte;
  uint8_t  sdop;
  uint8_t  vsdop;
} __attribute__((packed));

SBP_Header sbp_header = {30, 0xA0, 0xA2, 30, 0xFD, "ESP-GPS,0,unknown,unknown"};

SBP_frame sbp_frame;

uint8_t scaledByte(uint32_t value, uint32_t divisor)
{
  uint32_t scaled = value / divisor;
  if (scaled > 255) scaled = 255;
  return static_cast<uint8_t>(scaled);
}
}

void sbp_write_header(File& file)
{
  for (int i = 32; i < 64; i++) {
    ((uint8_t*)&sbp_header)[i] = 0xFF;
  }

  file.write((uint8_t*)&sbp_header, 64);
}

void sbp_write_frame(File& file)
{
  const uint32_t year = ubxMessage.navPvt.year;
  const uint8_t month = ubxMessage.navPvt.month;
  const uint8_t day = ubxMessage.navPvt.day;
  const uint8_t hour = ubxMessage.navPvt.hour;
  const uint8_t min = ubxMessage.navPvt.min;
  const uint8_t sec = ubxMessage.navPvt.sec;

  const uint8_t HDOP = scaledByte(ubxMessage.navDOP.hDOP + 1, 20);
  const uint8_t sdop = scaledByte(ubxMessage.navPvt.sAcc, 10);
  const uint8_t vsdop = scaledByte(ubxMessage.navPvt.vAcc, 10);

  sbp_frame.UtcSec = ubxMessage.navPvt.sec * 1000 + (ubxMessage.navPvt.nano + 500000) / 1000000;

  sbp_frame.date_time_UTC_packed =
    (((year - 2000) * 12 + month) << 22) |
    (day << 17) |
    (hour << 12) |
    (min << 6) |
    sec;

  sbp_frame.Lat = ubxMessage.navPvt.lat;
  sbp_frame.Lon = ubxMessage.navPvt.lon;
  sbp_frame.AltCM = ubxMessage.navPvt.hMSL / 10;
  sbp_frame.Sog = ubxMessage.navPvt.gSpeed / 10;          // stored in cm/sec in sbp file
  sbp_frame.Cog = ubxMessage.navPvt.heading / 1000;

  sbp_frame.SVIDCnt = ubxMessage.navPvt.numSV;
  const uint32_t numSV = 0xFFFFFFFF;
  sbp_frame.SVIDList =
    (ubxMessage.navPvt.numSV >= 32) ? 0xFFFFFFFF :
    (ubxMessage.navPvt.numSV == 0) ? 0 :
    (numSV >> (32 - ubxMessage.navPvt.numSV));

  sbp_frame.HDOP = HDOP;
  sbp_frame.ClmbRte = -ubxMessage.navPvt.velD / 10;
  sbp_frame.sdop = sdop;
  sbp_frame.vsdop = vsdop;

  file.write((uint8_t*)&sbp_frame, 32);
}
