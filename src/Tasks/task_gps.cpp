#include <Arduino.h>
#include "Core/Globals.h"
#include "tasks/task_gps.h"
#include "MANAGERS/config_manager.h"
#include "core/system_mode.h"
#include "Ublox/ublox.h"
#include "Display/E_paper.h"
#include "Storage/storage_manager.h"
#include "Storage/storage_session_log.h"
#include "Storage/Geojson.h"
#include "tasks/task_display.h"
#include "GPS/gps_simulator.h"


static void debugPrintStats()
{
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint < 1000) return; // 1 Hz
  lastPrint = millis();

  Serial.println(F("---- PERF STATS ----"));

  Serial.print(F("2s   (kn): "));
  Serial.println(S2.s_max_speed * MMPS_TO_KNOTS, 2);

  Serial.print(F("10s  (kn): "));
  Serial.println(S10.avg_5runs * MMPS_TO_KNOTS, 2);

  Serial.print(F("1h   (kn): "));
  Serial.println(S3600.s_max_speed * MMPS_TO_KNOTS, 2);

  Serial.print(F("NM   (kn): "));
  Serial.println(M1852.m_speed * MMPS_TO_KNOTS, 2);

  Serial.print(F("Alpha(kn): "));
  Serial.println(A500.alfa_speed_max * MMPS_TO_KNOTS, 2);

  Serial.print(F("Dist (m): "));
  Serial.println(total_distance * 0.001f, 1);

  Serial.print(F("Run  (m): "));
  Serial.println(Ublox.run_distance * 0.001f, 1);

  Serial.println();
}

// -----------------------------------------------------------------------------
// GPS task state
// -----------------------------------------------------------------------------
int GPS_delay=0;
TaskHandle_t t1=nullptr;

// Internal helpers
static void processGpsMessages(uint8_t msgType);

// -----------------------------------------------------------------------------
// GPS task (runs while logging / waiting for sats)
// -----------------------------------------------------------------------------
void taskOne(void *parameter)
{
  static uint8_t  lastSV=0;
  static uint32_t lastLogMs=0,lastSpeedUpdateMs=0,lastGeoJSONms=0;
  constexpr uint32_t LOG_INTERVAL_MS=2000;

  for(;;){
    if(getMode()!=MODE_LOGGING && getMode()!=MODE_WAIT_SATS){
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    wdt_task0=millis();

    int msg;
  #ifdef GPS_SIMULATOR
    msg=gps_simulator_step();
  #else
    msg=processGPS();
  #endif

    // -------------------------------------------------------------------------
    // NAV-PVT handling
    // -------------------------------------------------------------------------
    if(msg==MT_NAV_PVT){
      processGpsMessages(msg);
      Log_to_SD();

      // GeoJSON @ 1 Hz
      if(getMode()==MODE_LOGGING && GPS_Signal_OK){
        uint32_t now=millis();
        if(now-lastGeoJSONms>=1000){
          lastGeoJSONms=now;
          geojson_add_point(
            ubxMessage.navPvt.lat*1e-7,
            ubxMessage.navPvt.lon*1e-7
          );
        }
      }

      // Debug
      if(millis()-lastLogMs>=LOG_INTERVAL_MS){
        lastLogMs=millis();
        LOG_GPS("PVT","fix=%u sv=%u lat=%.6f lon=%.6f spd=%.2f",
          ubxMessage.navPvt.fixType,
          ubxMessage.navPvt.numSV,
          ubxMessage.navPvt.lat*1e-7,
          ubxMessage.navPvt.lon*1e-7,
          ubxMessage.navPvt.gSpeed*0.001f
        );
      }

      // WAIT_SATS → update SV count
      if(getMode()==MODE_WAIT_SATS){
        uint8_t sv=ubxMessage.navPvt.numSV;
        if(sv!=lastSV){
          lastSV=sv;
          screen_request_partial(0,100,250,122);
        }
      }
    }

    // -------------------------------------------------------------------------
    // Adaptive speed redraw
    // -------------------------------------------------------------------------
    if(getMode()==MODE_LOGGING && GPS_Signal_OK){
      float kts=ubxMessage.navPvt.gSpeed*MMPS_TO_KNOTS;
      uint32_t intervalMs =
        kts<10.0f ? UINT32_MAX :
        kts<20.0f ? 5000 :
        kts<38.0f ? 3000 : 1000;

      uint32_t now=millis();
      if(intervalMs!=UINT32_MAX && now-lastSpeedUpdateMs>=intervalMs){
        lastSpeedUpdateMs=now;
        screen_request_partial(0,0,250,122);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// -----------------------------------------------------------------------------
// UBX message processing
// -----------------------------------------------------------------------------
static void processGpsMessages(uint8_t msgType)
{
  static uint32_t timeWaitStartMs=0;

  if(msgType==MT_NAV_PVT){
    last_gps_msg=millis();

    if(GPS_Signal_OK) gps_speed_value=ubxMessage.navPvt.gSpeed;
    if(Time_Set_OK)   nav_pvt_message++;

    // -------------------------------------------------------------------------
    // First fix detection
    // -------------------------------------------------------------------------
    if(!GPS_Signal_OK &&
       ubxMessage.navPvt.numSV>=MIN_numSV_FIRST_FIX &&
       (ubxMessage.navPvt.sAcc/1000.0f)<MAX_Sacc_FIRST_FIX &&
       ubxMessage.navPvt.valid>=7)
    {
      GPS_Signal_OK=true;
      first_fix_GPS=millis()/1000;
      timeWaitStartMs=millis();
    }

    // WAIT_SATS → LOGGING
    if(GPS_Signal_OK && getMode()==MODE_WAIT_SATS)
      setMode(MODE_LOGGING);

    if(GPS_Signal_OK) GPS_delay++;

    // -------------------------------------------------------------------------
    // One-time GPS time sync (non-blocking, RP6-style)
    // -------------------------------------------------------------------------
    if(GPS_Signal_OK && !Time_Set_OK){
      if((ubxMessage.navPvt.valid & 0b011)==0b011){
        if(Set_GPS_Time(config.timezone)){
          Time_Set_OK=true;
          Shut_down_Save_session=true;
          start_logging_millis=millis();
          Open_files();
        }
      }else if(millis()-timeWaitStartMs>15000UL){
        // fail-safe: do NOT stall logging forever
        Time_Set_OK=true;
        start_logging_millis=millis();
        Open_files();
      }
    }

    // -------------------------------------------------------------------------
    // Main processing
    // -------------------------------------------------------------------------
    if(Time_Set_OK && nav_pvt_message>10 && nav_pvt_message!=old_message){
      old_message=nav_pvt_message;
      gps_speed_value=ubxMessage.navPvt.gSpeed;

      static uint32_t last_flush_time=0;
      if(millis()-last_flush_time>60000UL){
        Flush_files();
        last_flush_time=millis();
      }

      if(ubxMessage.navPvt.numSV<=MIN_numSV_GPS_SPEED_OK ||
         (ubxMessage.navPvt.sAcc/1000.0f)>MAX_Sacc_GPS_SPEED_OK ||
         (ubxMessage.navPvt.gSpeed/1000.0f)>MAX_GPS_SPEED_OK)
        gps_speed_value=0;

      Ublox.push_data(
        ubxMessage.navPvt.lat/1e7,
        ubxMessage.navPvt.lon/1e7,
        gps_speed_value
      );

      run_count=New_run_detection(
        ubxMessage.navPvt.heading/100000.0f,
        S2.avg_s
      );

      alfa_window=Alfa_indicator(
        M250,M100,
        ubxMessage.navPvt.heading/100000.0f
      );

      if(run_count!=old_run_count) Ublox.run_distance=0;
      old_run_count=run_count;

      M100.Update_distance(run_count);
      M250.Update_distance(run_count);
      M500.Update_distance(run_count);
      M1852.Update_distance(run_count);

      S2.Update_speed(run_count);
      s2.Update_speed(run_count);
      S10.Update_speed(run_count);
      s10.Update_speed(run_count);
      S1800.Update_speed(run_count);
      S3600.Update_speed(run_count);

      A250.Update_Alfa(M250);
      A500.Update_Alfa(M500);
      a500.Update_Alfa(M500);

      M_500.Update_Track();

      if(S2.avg_s>4000){
        S10_previous_run=S10.s_max_speed;
      }

        debugPrintStats();

    }
    return;
  }

  // ---------------------------------------------------------------------------
  // NAV-SAT
  // ---------------------------------------------------------------------------
  if(msgType==MT_NAV_SAT){
    nav_sat_message++;
    if(ubxMessage.navSatCount>UBX_MAX_SVS)
      ubxMessage.navSatCount=UBX_MAX_SVS;

    Ublox_Sat.push_SAT_info(
      ubxMessage.navSatHdr,
      ubxMessage.navSat,
      ubxMessage.navSatCount
    );
    return;
  }

  // NAV-DOP ignored (diagnostics only)
}
