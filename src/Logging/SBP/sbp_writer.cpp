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

uint32_t packedUtcDateTime()
{
  const uint32_t year = ubxMessage.navPvt.year;
  const uint8_t month = ubxMessage.navPvt.month;
  const uint8_t day = ubxMessage.navPvt.day;
  const uint8_t hour = ubxMessage.navPvt.hour;
  const uint8_t min = ubxMessage.navPvt.min;
  const uint8_t sec = ubxMessage.navPvt.sec;

  return (((year - 2000) * 12 + month) << 22) |
         (day << 17) |
         (hour << 12) |
         (min << 6) |
         sec;
}

uint32_t satelliteMask(uint8_t satelliteCount)
{
  if (satelliteCount >= 32) return 0xFFFFFFFF;
  if (satelliteCount == 0) return 0;
  return 0xFFFFFFFF >> (32 - satelliteCount);
}

SbpHeader makeSbpHeader()
{
  SbpHeader header = {30, 0xA0, 0xA2, 30, 0xFD, "ESP-GPS,0,unknown,unknown"};
  uint8_t* bytes = reinterpret_cast<uint8_t*>(&header);
  for (size_t i = 32; i < SBP_HEADER_SIZE; i++) {
    bytes[i] = 0xFF;
  }
  return header;
}

SbpFrame makeSbpFrame()
{
  SbpFrame frame {};
  frame.HDOP = scaledByte(ubxMessage.navDOP.hDOP + 1, 20);
  frame.SVIDCnt = ubxMessage.navPvt.numSV;
  frame.UtcSec = ubxMessage.navPvt.sec * 1000 + (ubxMessage.navPvt.nano + 500000) / 1000000;
  frame.date_time_UTC_packed = packedUtcDateTime();
  frame.SVIDList = satelliteMask(ubxMessage.navPvt.numSV);
  frame.Lat = ubxMessage.navPvt.lat;
  frame.Lon = ubxMessage.navPvt.lon;
  frame.AltCM = ubxMessage.navPvt.hMSL / 10;
  frame.Sog = currentStatsSogCms();
  frame.Cog = ubxMessage.navPvt.heading / 1000;
  frame.ClmbRte = -ubxMessage.navPvt.velD / 10;
  frame.sdop = scaledByte(ubxMessage.navPvt.sAcc, 10);
  frame.vsdop = scaledByte(ubxMessage.navPvt.vAcc, 10);
  return frame;
}
}

void sbp_write_header(File& file)
{
  const SbpHeader header = makeSbpHeader();
  file.write(reinterpret_cast<const uint8_t*>(&header), sizeof(header));
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
  const SbpFrame frame = makeSbpFrame();
  if (file.write(reinterpret_cast<const uint8_t*>(&frame), sizeof(frame)) == sizeof(frame)) {
    sbp_frame_count++;
    stampCurrentSampleWithSbpRow();
  }
}
