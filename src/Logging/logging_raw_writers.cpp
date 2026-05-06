#include "Logging/logging_raw_writers.h"

#include "Core/Globals.h"
#include "GPS/gps_runtime_state.h"
#include "Logging/sbp_writer.h"
#include "GPS/Ublox/ublox_driver.h"
#include "Config/config_types.h"

namespace {
uint32_t last_sbp_iTOW = 0;
int last_nav_sat_message = 0;

void checksumUpdate(uint8_t byte, uint8_t& ckA, uint8_t& ckB)
{
  ckA += byte;
  ckB += ckA;
}

void writeChecksumBytes(const uint8_t* bytes, size_t size, uint8_t& ckA, uint8_t& ckB)
{
  for (size_t i = 0; i < size; i++) checksumUpdate(bytes[i], ckA, ckB);
}

void writeUbxFrame(File& file, uint8_t cls, uint8_t id, const uint8_t* payload, uint16_t payloadSize)
{
  uint8_t ckA = 0;
  uint8_t ckB = 0;
  const uint8_t lenBytes[2] = {
    static_cast<uint8_t>(payloadSize & 0xFF),
    static_cast<uint8_t>(payloadSize >> 8)
  };

  checksumUpdate(cls, ckA, ckB);
  checksumUpdate(id, ckA, ckB);
  writeChecksumBytes(lenBytes, sizeof(lenBytes), ckA, ckB);
  writeChecksumBytes(payload, payloadSize, ckA, ckB);

  file.write(0xB5);
  file.write(0x62);
  file.write(cls);
  file.write(id);
  file.write(lenBytes, sizeof(lenBytes));
  file.write(payload, payloadSize);
  file.write(ckA);
  file.write(ckB);
}

void writeNavSatIfUpdated(File& file)
{
  if (nav_sat_message == last_nav_sat_message) return;

  last_nav_sat_message = nav_sat_message;

  const uint16_t payloadSize =
    sizeof(ubxMessage.navSatHdr.iTOW) +
    sizeof(ubxMessage.navSatHdr.version) +
    sizeof(ubxMessage.navSatHdr.numSvs) +
    sizeof(ubxMessage.navSatHdr.r1) +
    sizeof(ubxMessage.navSatHdr.r2) +
    (uint16_t)ubxMessage.navSatCount * sizeof(ubxMessage.navSat[0]);

  writeUbxFrame(
    file,
    0x01,
    0x35,
    reinterpret_cast<const uint8_t*>(&ubxMessage.navSatHdr.iTOW),
    payloadSize
  );
}

bool sbpLoggingReady(File& file)
{
  return config.logSBP && file;
}

bool sbpItowChanged()
{
  const uint32_t itow = ubxMessage.navPvt.iTOW;
  if (itow == last_sbp_iTOW) return false;

  last_sbp_iTOW = itow;
  return true;
}
}

void logging_raw_writers_reset()
{
  last_sbp_iTOW = 0;
  last_nav_sat_message = 0;
}

void logging_raw_writers_write_ubx(File& ubxfile)
{
  if (config.logUBX && ubxfile) {
    writeUbxFrame(
      ubxfile,
      0x01,
      0x07,
      reinterpret_cast<const uint8_t*>(&ubxMessage.navPvt),
      sizeof(ubxMessage.navPvt)
    );
    writeNavSatIfUpdated(ubxfile);
  }

  if (config.logUBX_nav_sat && ubxfile) {
    writeUbxFrame(
      ubxfile,
      0x01,
      0x04,
      reinterpret_cast<const uint8_t*>(&ubxMessage.navDOP.iTOW),
      sizeof(ubxMessage.navDOP) - 4
    );
  }
}

void logging_raw_writers_write_sbp(File& sbpfile)
{
  if (!sbpLoggingReady(sbpfile)) return;
  if (!sbpItowChanged()) return;

  sbp_write_frame(sbpfile);
}
