#include "Storage/session_raw_writers.h"

#include "Core/Globals.h"
#include "Storage/sbp.h"
#include "Ublox/Ublox.h"
#include "Core/system_mode.h"
#include "managers/config_types.h"

namespace {
uint32_t last_sbp_iTOW = 0;
int last_nav_sat_message = 0;
}

void session_raw_writers_reset()
{
  last_sbp_iTOW = 0;
  last_nav_sat_message = 0;
}

void session_write_ubx(File& ubxfile)
{
  if (config.logUBX && ubxfile) {
    ubxfile.write(0xB5);
    ubxfile.write(0x62);
    ubxfile.write((const uint8_t*)&ubxMessage.navPvt, sizeof(ubxMessage.navPvt));

    if (nav_sat_message != last_nav_sat_message) {
      last_nav_sat_message = nav_sat_message;
      ubxfile.write(0xB5);
      ubxfile.write(0x62);
      ubxfile.write(
        (const uint8_t*)&ubxMessage.navSat,
        (ubxMessage.navSatHdr.len + 6)
      );
    }
  }

  if (config.logUBX_nav_sat && ubxfile) {
    ubxfile.write(0xB5);
    ubxfile.write(0x62);
    ubxfile.write((const uint8_t*)&ubxMessage.navDOP, sizeof(ubxMessage.navDOP));
  }
}

void session_write_sbp(File& sbpfile)
{
  if (!config.logSBP || !sbpfile || getMode() != MODE_LOGGING) return;

  uint32_t itow = ubxMessage.navPvt.iTOW;
  if (itow == last_sbp_iTOW) return;

  last_sbp_iTOW = itow;
  log_SBP(sbpfile);
}
