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
// Safety
// -----------------------------------------------------------------------------
#define VALID_GPS_INDEX(i) ((i) >= 0 && (i) < BUFFER_SIZE)

// -----------------------------------------------------------------------------
// State / buffers
// -----------------------------------------------------------------------------
char dataStr[255] = "", Buffer[50] = "";
uint64_t GPS_UTC_ms;
static uint32_t last_sbp_iTOW = 0;

// Files
File ubxfile, sbpfile;

// Filenames
char filenameERR[128] = "/", filenameUBX[128] = "/", filenameSBP[128] = "/", filenameGEO[128] = "/";

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

  snprintf(path, sizeof(path), "/logs/%s", base);

  snprintf(filenameERR, sizeof(filenameERR), "%s.txt", path);
  snprintf(filenameUBX, sizeof(filenameUBX), "%s.ubx", path);
  snprintf(filenameSBP, sizeof(filenameSBP), "%s.sbp", path);
  snprintf(filenameGEO, sizeof(filenameGEO), "%s.geojson", path);

  if(config.logUBX)
    ubxfile = SD_MMC.open(filenameUBX, FILE_APPEND);

  if(config.logSBP){
    sbpfile = SD_MMC.open(filenameSBP, FILE_WRITE);
    if(sbpfile.size() == 0)
      log_header_SBP(sbpfile);
  }

  // ---- GeoJSON base track ----
  geojson_begin(filenameGEO);
  geojson_begin_feature("track");

  LOG_STORAGE("LOG","Session started %s", base);
}

// -----------------------------------------------------------------------------
// Periodic flush (load-balanced)
// -----------------------------------------------------------------------------
void Flush_files(void)
{
  if(storage_shutting_down || systemInfo.sample_rate > 10) return;

  static uint8_t lb = 0;
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
    ubxfile.write((const uint8_t*)&ubxMessage.navPvt, sizeof(ubxMessage.navPvt));

    static int old_sat = 0;
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
    ubxfile.write((const uint8_t*)&ubxMessage.navDOP, sizeof(ubxMessage.navDOP));
  }

  if(config.logSBP && sbpfile && getMode() == MODE_LOGGING){
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
  Serial.println("[STORAGE] Close_files()");

  // ------------------------------------------------------------
  // FINAL SESSION STATS
  // ------------------------------------------------------------
  GeoJSONStats s {
    .nm       = RTC_mile_knots,
    .alpha    = RTC_alp_knots,
    .h1       = RTC_1h_knots,
    .max      = RTC_max_2s_knots,
    .avg10    = RTC_avg_10s_knots,
    .distance = RTC_distance
  };
  geojson_set_stats(s);

  // ---- Finish base track ----
  geojson_end_feature();

  // ------------------------------------------------------------
  // Helpers (ring-safe, 1 Hz decimation)
  // ------------------------------------------------------------
  auto add_ring_window_1hz = [&](int start_gps_idx, int seconds){
    for(int s = 0; s < seconds; s++){
      int idx = start_gps_idx + s * systemInfo.sample_rate;
      idx %= BUFFER_SIZE; if(idx < 0) idx += BUFFER_SIZE;
      if(VALID_GPS_INDEX(idx))
        geojson_add_point(_lat[idx], _long[idx]);
    }
  };

  auto add_ring_range_1hz = [&](int start, int end){
    int idx = start;
    int step = 0;

    for(int guard = 0; guard < BUFFER_SIZE; guard++){
      if(step % systemInfo.sample_rate == 0){
        if(VALID_GPS_INDEX(idx))
          geojson_add_point(_lat[idx], _long[idx]);
      }
      if(idx == end) break;
      idx++; if(idx >= BUFFER_SIZE) idx = 0;
      step++;
    }
  };

  // ------------------------------------------------------------
  // DERIVED FEATURES (DISPLAY-RATE GEOMETRY)
  // ------------------------------------------------------------

  // ---- 2s (fixed length) ----
  if(win_2s_start >= 0){
    geojson_begin_feature("2s");
    add_ring_window_1hz(win_2s_start, 2);
    geojson_end_feature();
  }

  // ---- Top-5 × 10s (fixed length) ----
  for(int i = 0; i < win_10s_top5_count; i++){
    int s = win_10s_top5_start[i];
    if(s < 0) continue;

    geojson_begin_feature("10s");
    add_ring_window_1hz(s, 10);
    geojson_end_feature();
  }

  // ---- Alpha (geometry-defined) ----
  if(alpha_start >= 0 && alpha_end >= 0){
    geojson_begin_feature("alpha");
    add_ring_range_1hz(alpha_start, alpha_end);
    geojson_end_feature();
  }

  // ---- Nautical Mile (geometry-defined) ----
  if(win_nm_start >= 0 && win_nm_end >= 0){
    geojson_begin_feature("nm");
    add_ring_range_1hz(win_nm_start, win_nm_end);
    geojson_end_feature();
  }

  // ---- 1h (already 1 Hz by definition) ----
  if(win_1h_start_sec >= 0 && win_1h_end_sec > win_1h_start_sec){
    geojson_begin_feature("1h");
    for(int s = win_1h_start_sec; s <= win_1h_end_sec; s++){
      int idx = sec_to_gps_index[s];
      if(VALID_GPS_INDEX(idx))
        geojson_add_point(_lat[idx], _long[idx]);
    }
    geojson_end_feature();
  }

  // ------------------------------------------------------------
  // FINALISE GEOJSON
  // ------------------------------------------------------------
  geojson_end();

  // ------------------------------------------------------------
  // FLUSH + CLOSE RAW FILES
  // ------------------------------------------------------------
  if(ubxfile){
    ubxfile.flush();
    ubxfile.close();
    ubxfile = File();
  }

  if(sbpfile){
    sbpfile.flush();
    sbpfile.close();
    sbpfile = File();
  }

  Serial.println("[STORAGE] files closed");
}
