#include "Storage/sbp.h"
#include "Ublox/Ublox.h"
#include "Globals.h"
#include <SD_MMC.h>
#include <dirent.h>
#include <sys/stat.h>

static bool sbp_test_done = false;

SBP_Header sbp_header = {30, 0xA0, 0xA2, 30, 0xFD, "ESP-GPS,0,unknown,unknown"};

SBP_frame sbp_frame;

void log_header_SBP(File &file){
  for(int i=32;i<64;i++) ((uint8_t*)&sbp_header)[i]=0xFF;
  file.write((uint8_t*)&sbp_header,64);
}

void log_SBP(File &file)
{
  if(ubxMessage.navPvt.fixType<3) return;

  const uint32_t year=ubxMessage.navPvt.year;
  const uint8_t month=ubxMessage.navPvt.month,day=ubxMessage.navPvt.day;
  const uint8_t hour=ubxMessage.navPvt.hour,min=ubxMessage.navPvt.min,sec=ubxMessage.navPvt.sec;

  uint32_t HDOP=(ubxMessage.navDOP.hDOP+1)/20; if(HDOP>255) HDOP=255;
  uint32_t sdop=ubxMessage.navPvt.sAcc/10;     if(sdop>255) sdop=255;
  uint32_t vsdop=ubxMessage.navPvt.vAcc/10;    if(vsdop>255) vsdop=255;

  sbp_frame.UtcSec=ubxMessage.navPvt.iTOW;

  sbp_frame.date_time_UTC_packed=(((year-2000)*12+month)<<22) | (day<<17) | (hour<<12) | (min<<6)|sec;

  sbp_frame.Lat=ubxMessage.navPvt.lat;
  sbp_frame.Lon=ubxMessage.navPvt.lon;
  sbp_frame.AltCM=ubxMessage.navPvt.hMSL/10;
  sbp_frame.Sog=ubxMessage.navPvt.gSpeed/10;
  sbp_frame.Cog=ubxMessage.navPvt.heading/1000;

  sbp_frame.SVIDCnt=ubxMessage.navPvt.numSV;
  sbp_frame.SVIDList = (sbp_frame.SVIDCnt==0)?0:(sbp_frame.SVIDCnt>=32)?0xFFFFFFFF:((1UL<<sbp_frame.SVIDCnt)-1);

  sbp_frame.HDOP=HDOP;
  sbp_frame.ClmbRte=-ubxMessage.navPvt.velD/10;
  sbp_frame.sdop=sdop;
  sbp_frame.vsdop=vsdop;

  file.write((uint8_t*)&sbp_frame,32);
}
