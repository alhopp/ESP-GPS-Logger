#include "Storage/sbp.h"
#include "Ublox/Ublox.h"
#include "Globals.h"

#include <SD_MMC.h>

#include <dirent.h>
#include <sys/stat.h>
static bool sbp_test_done = false;


SBP_Header sbp_header = {
  30,
  0xA0,
  0xA2,
  30,
  0xFD,
  "ESP-GPS,0,unknown,unknown"
};

SBP_frame sbp_frame;

void log_header_SBP(File &file) {
  for (int i = 32; i < 64; i++)
    ((uint8_t*)&sbp_header)[i] = 0xFF;

  file.write((uint8_t*)&sbp_header, 64);
}



void log_SBP(File &file)

{

  if (sbp_test_done)
    return;

  sbp_test_done = true;



// Make sure SD is mounted
if (!SD_MMC.cardType()) {
  LOG_ERROR("TEST", "SD_MMC not available");
  return;
}

// Make sure /logs exists
if (!SD_MMC.exists("/logs")) {
  SD_MMC.mkdir("/logs");
}

LOG_STORAGE("TEST", "log_SBP → writing 10 test files");

char path[64];

for (int i = 1; i <= 10; i++) {
  snprintf(path, sizeof(path), "/logs/sbp_test_%02d.txt", i);

  File f = SD_MMC.open(path, FILE_WRITE);
  if (!f) {
    LOG_ERROR("TEST", "open failed: %s", path);
    continue;
  }

  f.println("SBP FILE MANAGER TEST");
  f.printf("File number : %d\n", i);
  f.printf("Millis      : %lu\n", millis());
  f.println("--------------------------------");
  f.println("If this file appears in the UI,");
  f.println("the SD file manager works.");
  f.close();
}

}

/*
void log_SBP(File &file)
{
  if (ubxMessage.navPvt.fixType < 3)
    return;

  uint32_t year  = ubxMessage.navPvt.year;
  uint8_t  month = ubxMessage.navPvt.month;
  uint8_t  day   = ubxMessage.navPvt.day;
  uint8_t  hour  = ubxMessage.navPvt.hour;
  uint8_t  min   = ubxMessage.navPvt.min;
  uint8_t  sec   = ubxMessage.navPvt.sec;

  uint32_t HDOP = (ubxMessage.navDOP.hDOP + 1) / 20;
  if (HDOP > 255) HDOP = 255;

  uint32_t sdop = ubxMessage.navPvt.sAcc / 10;
  if (sdop > 255) sdop = 255;

  uint32_t vsdop = ubxMessage.navPvt.vAcc / 10;
  if (vsdop > 255) vsdop = 255;

  //uint32_t ms =
  //  ubxMessage.navPvt.sec * 1000 +
  //  (ubxMessage.navPvt.nano + 500000) / 1000000;
  //if (ms > 59999) ms = 59999;
  //sbp_frame.UtcSec = ms;
 
  sbp_frame.UtcSec = ubxMessage.navPvt.iTOW;

  sbp_frame.date_time_UTC_packed =
    (((year - 2000) * 12 + month) << 22) |
    (day  << 17) |
    (hour << 12) |
    (min  << 6) |
    sec;

  sbp_frame.Lat = ubxMessage.navPvt.lat;
  sbp_frame.Lon = ubxMessage.navPvt.lon;
  sbp_frame.AltCM = ubxMessage.navPvt.hMSL / 10;
  sbp_frame.Sog   = ubxMessage.navPvt.gSpeed / 10;
  sbp_frame.Cog   = ubxMessage.navPvt.heading / 1000;

  sbp_frame.SVIDCnt = ubxMessage.navPvt.numSV;
  if (sbp_frame.SVIDCnt == 0) {
   sbp_frame.SVIDList = 0;
    } else if (sbp_frame.SVIDCnt >= 32) {
  sbp_frame.SVIDList = 0xFFFFFFFF;
    } else {
  sbp_frame.SVIDList = (1UL << sbp_frame.SVIDCnt) - 1;
}


  sbp_frame.HDOP = HDOP;
  sbp_frame.ClmbRte = -ubxMessage.navPvt.velD / 10;
  sbp_frame.sdop = sdop;
  sbp_frame.vsdop = vsdop;

  file.write((uint8_t*)&sbp_frame, 32);
}


*/