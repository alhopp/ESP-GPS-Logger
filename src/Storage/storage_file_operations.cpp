// -----------------------------------------------------------------------------
// File Operations Manager
// Opens / flushes / logs / closes UBX, SBP, TXT (+ others) session files.
// ----------------------------------------------------------------------------- 

#include <Arduino.h>
#include <FS.h>

#include "Storage/sbp.h"
#include "Storage/geojson.h"
#include <SD_MMC.h>

#include "Storage/storage_manager.h"

#include "MANAGERS/config_manager.h"
#include "core/system_mode.h"
#include "Ublox/Ublox.h"
#include "Core/Globals.h"
#include "Core/Definitions.h"

#include "Core/system_info.h"
#include "Core/rtc_state.h"

#include "GPS/gps_speed.h"
#include "GPS/gps_time.h"
#include "GPS/gps_alpha.h"

#include <esp_system.h>

// -----------------------------------------------------------------------------
// State / buffers
// -----------------------------------------------------------------------------
char dataStr[255]="", Buffer[50]="";
uint64_t GPS_UTC_ms;
static uint32_t last_sbp_iTOW=0;

// Files
File ubxfile, sbpfile;

// Filenames
char filenameERR[128]="/", filenameUBX[128]="/", filenameSBP[128]="/", filenameGEO[128]="/";  

// -----------------------------------------------------------------------------
// Open logging files
// -----------------------------------------------------------------------------
void Open_files(void)
{
  if(storage_shutting_down || !Time_Set_OK){
    LOG_STORAGE("Open_files","called without valid GPS time");
    return;
  }

  if(!SD_MMC.exists("/logs")) SD_MMC.mkdir("/logs");
  getLocalTime(&tmstruct);

  uint64_t chipMac = 0;
  esp_efuse_mac_get_default((uint8_t*)&chipMac);

  uint8_t mac3 = (chipMac >> 16) & 0xFF;
  uint8_t mac4 = (chipMac >> 8)  & 0xFF;
  uint8_t mac5 = (chipMac)       & 0xFF;

  char base[96], path[128];
  snprintf(base, sizeof(base),
    "%02X%02X%02X_%04d%02d%02d_%02d%02d%02d",
    mac3, mac4, mac5,
    tmstruct.tm_year + 1900,
    tmstruct.tm_mon  + 1,
    tmstruct.tm_mday,
    tmstruct.tm_hour,
    tmstruct.tm_min,
    tmstruct.tm_sec
  );

  snprintf(path,sizeof(path),"/logs/%s",base);

  snprintf(filenameERR,sizeof(filenameERR),"%s.txt",path);
  snprintf(filenameUBX,sizeof(filenameUBX),"%s.ubx",path);
  snprintf(filenameSBP,sizeof(filenameSBP),"%s.sbp",path);
  snprintf(filenameGEO,sizeof(filenameGEO),"%s.geojson",path);

  if(config.logUBX)
    ubxfile = SD_MMC.open(filenameUBX, FILE_APPEND);

  if(config.logSBP){
    sbpfile = SD_MMC.open(filenameSBP, FILE_WRITE);
    if(sbpfile.size()==0)
      log_header_SBP(sbpfile);
  }

  // ---- GeoJSON base track ----
  geojson_begin(filenameGEO);
  geojson_begin_feature("track");

  LOG_STORAGE("LOG","Session started %s",base);
}

// -----------------------------------------------------------------------------
// Periodic flush (load-balanced)
// -----------------------------------------------------------------------------
void Flush_files(void)
{
  if(storage_shutting_down || systemInfo.sample_rate > 10) return;

  static uint8_t lb=0;
  switch(lb){
    case 0: if(ubxfile) ubxfile.flush(); break;
    case 3: if(sbpfile) sbpfile.flush(); break;
  }
  lb = (lb + 1) % 5;
}

// -----------------------------------------------------------------------------
// Write logging data
// -----------------------------------------------------------------------------
void Log_to_SD(void)
{
  if(storage_shutting_down || !Time_Set_OK) return;

  if(config.logUBX && ubxfile){
    ubxfile.write(0xB5); ubxfile.write(0x62);
    ubxfile.write((const uint8_t*)&ubxMessage.navPvt,sizeof(ubxMessage.navPvt));

    static int old_sat=0;
    if(nav_sat_message != old_sat){
      old_sat = nav_sat_message;
      ubxfile.write(0xB5); ubxfile.write(0x62);
      ubxfile.write(
        (const uint8_t*)&ubxMessage.navSat,
        (ubxMessage.navSatHdr.len + 6)
      );
    }
  }

  if(config.logUBX_nav_sat && ubxfile){
    ubxfile.write(0xB5); ubxfile.write(0x62);
    ubxfile.write((const uint8_t*)&ubxMessage.navDOP,sizeof(ubxMessage.navDOP));
  }

  if(config.logSBP && sbpfile && getMode()==MODE_LOGGING){
    uint32_t itow = ubxMessage.navPvt.iTOW;
    if(itow != last_sbp_iTOW){
      last_sbp_iTOW = itow;
      log_SBP(sbpfile);
    }
  }
}

// -----------------------------------------------------------------------------
// Close files cleanly + write derived GeoJSON features
// -----------------------------------------------------------------------------
void Close_files(void)
{
  // ---- Final session stats (SET FIRST) ----
  GeoJSONStats s {
    .nm       = RTC_mile_knots,
    .alpha    = RTC_alp_knots,
    .h1       = RTC_1h_knots,
    .max      = RTC_max_2s_knots,
    .avg10    = RTC_avg_10s_knots,
    .distance = RTC_distance
  };
  geojson_set_stats(s);

  // ---- Finish base track (stats written here) ----
  geojson_end_feature();

  // ============================================================
  // DERIVED LINESTRINGS
  // ============================================================

  if(win_2s_start >= 0 && win_2s_end >= win_2s_start){
    geojson_begin_feature("2s");
    for(int i = win_2s_start; i <= win_2s_end; i++)
      geojson_add_point(_lat[i], _long[i]);
    geojson_end_feature();
  }

  if(win_10s_start >= 0 && win_10s_end >= win_10s_start){
    geojson_begin_feature("10s");
    for(int i = win_10s_start; i <= win_10s_end; i++)
      geojson_add_point(_lat[i], _long[i]);
    geojson_end_feature();
  }

  if(alpha_start >= 0 && alpha_end >= alpha_start){
    geojson_begin_feature("alpha");
    for(int i = alpha_start; i <= alpha_end; i++)
      geojson_add_point(_lat[i], _long[i]);
    geojson_end_feature();
  }

  if(win_nm_start >= 0 && win_nm_end >= win_nm_start){
    geojson_begin_feature("nm");
    for(int i = win_nm_start; i <= win_nm_end; i++)
      geojson_add_point(_lat[i], _long[i]);
    geojson_end_feature();
  }

  if(win_1h_start_sec >= 0 && win_1h_end_sec > win_1h_start_sec){
    geojson_begin_feature("1h");
    for(int s = win_1h_start_sec; s <= win_1h_end_sec; s++){
      int idx = sec_to_gps_index[s];
      geojson_add_point(_lat[idx], _long[idx]);
    }
    geojson_end_feature();
  }

  geojson_end();
}
