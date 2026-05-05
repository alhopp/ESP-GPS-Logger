#include "Logging/session_raw_writers.h"

#include "Core/Globals.h"
#include "Logging/sbp.h"
#include "GPS/Ublox/ublox_driver.h"
#include "Core/system_mode.h"
#include "Config/config_types.h"

namespace {
uint32_t last_sbp_iTOW = 0;
int last_nav_sat_message = 0;

void writeUbxSync(File& file)
{
  file.write(0xB5);
  file.write(0x62);
}

void writeUbxMessage(File& file, const void* payload, size_t payloadSize)
{
  writeUbxSync(file);
  file.write(static_cast<const uint8_t*>(payload), payloadSize);
}

void writeNavSatIfUpdated(File& file)
{
  if (nav_sat_message == last_nav_sat_message) return;

  last_nav_sat_message = nav_sat_message;
  writeUbxMessage(file, &ubxMessage.navSat, ubxMessage.navSatHdr.len + 6);
}

bool sbpLoggingReady(File& file)
{
  return config.logSBP && file && getMode() == MODE_LOGGING;
}

bool sbpItowChanged()
{
  const uint32_t itow = ubxMessage.navPvt.iTOW;
  if (itow == last_sbp_iTOW) return false;

  last_sbp_iTOW = itow;
  return true;
}
}

void session_raw_writers_reset()
{
  last_sbp_iTOW = 0;
  last_nav_sat_message = 0;
}

void session_raw_writers_write_ubx(File& ubxfile)
{
  if (config.logUBX && ubxfile) {
    writeUbxMessage(ubxfile, &ubxMessage.navPvt, sizeof(ubxMessage.navPvt));
    writeNavSatIfUpdated(ubxfile);
  }

  if (config.logUBX_nav_sat && ubxfile) {
    writeUbxMessage(ubxfile, &ubxMessage.navDOP, sizeof(ubxMessage.navDOP));
  }
}

void session_raw_writers_write_sbp(File& sbpfile)
{
  if (!sbpLoggingReady(sbpfile)) return;
  if (!sbpItowChanged()) return;

  sbp_write_frame(sbpfile);
}
