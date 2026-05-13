// ============================================================================
// sbp_writer.cpp
//
// Implements the project SBP binary log writer. The packed record layout here
// defines the on-device SBP file format and must remain compatible with readers.
// ============================================================================

#include "Logging/SBP/sbp_writer.h"
#include "Logging/SBP/sbp_format.h"

#include "GPS/Ublox/ublox_driver.h"
#include "GPS/Data/gps_data.h"
#include "Core/Globals.h"

namespace {
SbpHeader sbp_header = {30, 0xA0, 0xA2, 30, 0xFD, "ESP-GPS,0,unknown,unknown"};

SbpFrame sbp_frame;
int sbp_frame_count = 0;

uint8_t scaledByte(uint32_t value, uint32_t divisor)
{
  uint32_t scaled = value / divisor;
  if (scaled > 255) scaled = 255;
  return static_cast<uint8_t>(scaled);
}

uint16_t currentStatsSogCms()
{
  if (index_GPS < 0) return static_cast<uint16_t>(ubxMessage.navPvt.gSpeed / 10);
  return _sogCms[index_GPS % BUFFER_SIZE];
}

void stampCurrentSampleWithSbpRow()
{
  if (index_GPS < 0) return;
  _sbpIndex[index_GPS % BUFFER_SIZE] = sbp_frame_count;
}
}

void sbp_write_header(File& file)
{
  for (size_t i = 32; i < SBP_HEADER_SIZE; i++) {
    ((uint8_t*)&sbp_header)[i] = 0xFF;
  }

  file.write((uint8_t*)&sbp_header, SBP_HEADER_SIZE);
}

void sbp_writer_reset()
{
  sbp_frame_count = 0;
}

int sbp_writer_frame_count()
{
  return sbp_frame_count;
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
  sbp_frame.Sog = currentStatsSogCms();                    // stored in cm/sec in sbp file
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

  if (file.write((uint8_t*)&sbp_frame, SBP_FRAME_SIZE) == SBP_FRAME_SIZE) {
    sbp_frame_count++;
    stampCurrentSampleWithSbpRow();
  }
}
