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
#include "GPS/gps_alpha.h"
#include "GPS/gps_track.h"

// -----------------------------------------------------------------------------
// Session state (MUST be above usage)
// -----------------------------------------------------------------------------
static bool session_active = false;

static void debugPrintStats()
{
  static uint32_t lastPrint = 0;
  if(millis() - lastPrint < 1000) return;
  lastPrint = millis();

  Serial.println(F("---- PERF STATS ----"));

  bool first;
  int  n;

  // ---------------- 2s (Top 5, rolling; worst→best) ----------------
  Serial.print(F("2s   (kn):   "));
  first = true; n = 0;
  for(int i=9;i>=0 && n<5;i--){
    if(S2.avg_speed[i] > 0){
      if(!first) Serial.print(F(" | "));
      Serial.print((double)S2.avg_speed[i] * CMPS_TO_KNOTS, 3);
      first = false; n++;
    }
  }
  Serial.println();

  // ---------------- 10s (Top 5, rolling; worst→best) ----------------
  Serial.print(F("10s  (kn):   "));
  first = true; n = 0;
  for(int i=9;i>=0 && n<5;i--){
    if(S10.avg_speed[i] > 0){
      if(!first) Serial.print(F(" | "));
      Serial.print((double)S10.avg_speed[i] * CMPS_TO_KNOTS, 3);

      first = false; n++;
    }
  }
  Serial.println();

  // ---------------- 1h (scalar) ----------------
  Serial.print(F("1h   (kn):   "));
  Serial.println((double)S3600.s_max_speed * CMPS_TO_KNOTS, 3);


  // ---------------- NM (Top 5, rolling; worst→best) ----------------
  Serial.print(F("NM   (kn):   "));
  first = true; n = 0;
  for(int i=9;i>=0 && n<5;i--){
    if(M1852.avg_speed[i] > 0){
      if(!first) Serial.print(F(" | "));
      Serial.print((double)M1852.avg_speed[i] * CMPS_TO_KNOTS, 3);
      first = false; n++;
    }
  }
  Serial.println();

  // ---------------- Alpha (Top 5; best→worst) ----------------
  Serial.print(F("Alpha(kn):   "));
  first = true; n = 0;
  for(int i=0;i<10 && n<5;i++){
    if(A500.avg_speed[i] > 0){
      if(!first) Serial.print(F(" | "));
      Serial.print((double)A500.avg_speed[i] * CMPS_TO_KNOTS, 3);

      first = false; n++;
    }
  }
  Serial.println();

  // ---------------- Distances ----------------
  Serial.print(F("Dist (km):   "));
  Serial.println(total_distance, 3); 


  Serial.print(F("Run  (km):   "));
  Serial.println(Ublox.run_distance, 3);

  Serial.print(F("Run #:        "));
  Serial.println(run_count);

  Serial.println();
}

// -----------------------------------------------------------------------------
// GPS task state
// -----------------------------------------------------------------------------
int GPS_delay = 0;
TaskHandle_t t1 = nullptr;

// Internal helpers
static void processGpsMessages(uint8_t msgType);

// -----------------------------------------------------------------------------
// GPS task
// -----------------------------------------------------------------------------
void taskOne(void *parameter)
{
  static uint8_t  lastSV = 0;
  static uint32_t lastLogMs = 0, lastSpeedUpdateMs = 0, lastGeoJSONms = 0;
  constexpr uint32_t LOG_INTERVAL_MS = 2000;

  for (;;) {
    if (getMode() != MODE_LOGGING && getMode() != MODE_WAIT_SATS) {
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    wdt_task0 = millis();

    int msg;
#ifdef GPS_SIMULATOR
    msg = gps_simulator_step();
#else
    msg = processGPS();
#endif

    if (msg == MT_NAV_PVT) {
      processGpsMessages(msg);

      // ---- logging + geojson ONLY after session start ----
    if (session_active && GPS_Signal_OK) {
        Log_to_SD();

        if (getMode() == MODE_LOGGING) {
          uint32_t now = millis();
          if (now - lastGeoJSONms >= 1000) {
            lastGeoJSONms = now;
            geojson_add_point(
              ubxMessage.navPvt.lat * 1e-7,
              ubxMessage.navPvt.lon * 1e-7
            );
          }
        }
      }

    /*
      if (millis() - lastLogMs >= LOG_INTERVAL_MS) {
        lastLogMs = millis();
        LOG_GPS("PVT","fix=%u sv=%u lat=%.6f lon=%.6f spd=%.2f",
          ubxMessage.navPvt.fixType,
          ubxMessage.navPvt.numSV,
          ubxMessage.navPvt.lat * 1e-7,
          ubxMessage.navPvt.lon * 1e-7,
          ubxMessage.navPvt.gSpeed * 0.001f
        );
      }
    */

      if (getMode() == MODE_WAIT_SATS) {
        uint8_t sv = ubxMessage.navPvt.numSV;
        if (sv != lastSV) {
          lastSV = sv;
          screen_request_partial(0, 100, 250, 122);
        }
      }
    }

    if (getMode() == MODE_LOGGING && GPS_Signal_OK) {
      float kts = ubxMessage.navPvt.gSpeed * MMPS_TO_KNOTS;
      uint32_t intervalMs =
        kts < 10.0f ? UINT32_MAX :
        kts < 20.0f ? 5000 :
        kts < 38.0f ? 3000 : 1000;

      uint32_t now = millis();
      if (intervalMs != UINT32_MAX && now - lastSpeedUpdateMs >= intervalMs) {
        lastSpeedUpdateMs = now;
        screen_request_partial(0, 0, 250, 122);
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
  static uint32_t timeWaitStartMs = 0;

  if (msgType != MT_NAV_PVT) return;

  last_gps_msg = millis();

  if (session_active) nav_pvt_message++;



  if (!GPS_Signal_OK &&
      ubxMessage.navPvt.numSV >= MIN_numSV_FIRST_FIX &&
      (ubxMessage.navPvt.sAcc / 1000.0f) < MAX_Sacc_FIRST_FIX &&
      ubxMessage.navPvt.valid >= 7) {
    GPS_Signal_OK = true;
    first_fix_GPS = millis() / 1000;
    timeWaitStartMs = millis();
  }

  if (GPS_Signal_OK && getMode() == MODE_WAIT_SATS)
    setMode(MODE_LOGGING);

if (GPS_Signal_OK && !Time_Set_OK && !session_active) {
  if ((ubxMessage.navPvt.valid & 0b011) == 0b011 ||
      millis() - timeWaitStartMs > 15000UL) {

    // Optional: keep RP6 behaviour if you want
    if ((ubxMessage.navPvt.valid & 0b011) == 0b011) Set_GPS_Time(config.timezone);

    Time_Set_OK = true;
    Shut_down_Save_session = true;
    start_logging_millis = millis();

    reset_session_stats();     // MUST be first
    Open_files();              // SBP starts after reset
    session_active = true;     // enable stats + sbp together
  }
}


  if (!session_active || nav_pvt_message == old_message)
    return;

  old_message = nav_pvt_message;
  gps_speed_value = ubxMessage.navPvt.gSpeed;

  Ublox.push_data(
    ubxMessage.navPvt.lat / 1e7,
    ubxMessage.navPvt.lon / 1e7,
    gps_speed_value
  );

  run_count = New_run_detection(
    ubxMessage.navPvt.heading / 100000.0f,
    S2.avg_s
  );

  alfa_window = Alfa_indicator(
    M250, M100,
    ubxMessage.navPvt.heading / 100000.0f
  );

  if (run_count != old_run_count)
    Ublox.run_distance = 0;

  old_run_count = run_count;

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

  debugPrintStats();
}
