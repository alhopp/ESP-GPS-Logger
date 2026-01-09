#include <Arduino.h>

#include "Globals.h"
#include "task_gps.h"
#include "config_manager.h"
#include "Storage/storage_manager.h"
#include "system_mode.h"

#include "Ublox/ublox.h"
#include "Storage/storage_session_log.h"
#include "ESP_functions.h"
#include "Display/E_paper.h"
#include "task_display.h"

#include <SD_MMC.h>
#include "Definitions.h"

#include "gps_simulator.h"

// --------------------------------------------------
// Runtime state
// --------------------------------------------------
int GPS_delay = 0;

// --------------------------------------------------
// Task handle
// --------------------------------------------------
TaskHandle_t t1 = nullptr;

// --------------------------------------------------
// External state
// --------------------------------------------------
extern bool sleep_mode;

// --------------------------------------------------
// Forward declarations
// --------------------------------------------------
static void processGpsMessages();

// --------------------------------------------------
// GPS task
// --------------------------------------------------
void taskOne(void *parameter)
{
  uint32_t lastLog = 0;
  static uint8_t lastSV = 0;

  static uint32_t lastLogMs = 0;
  constexpr uint32_t LOG_INTERVAL_MS = 2000;

  for (;;)
  {
    if (getMode() != MODE_LOGGING &&
        getMode() != MODE_WAIT_SATS &&
        getMode() != MODE_WIFI_SOFT_AP)
    {
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    wdt_task0 = millis();

    #ifdef GPS_SIMULATOR
        int msg = gps_simulator_step();
    #else
        int msg = processGPS();
    #endif

    if (msg == MT_NAV_PVT)
    {
      if (millis() - lastLogMs >= LOG_INTERVAL_MS)
          {
            lastLogMs = millis();

            LOG_GPS("PVT",
              "fix=%u sv=%u lat=%.6f lon=%.6f spd=%.2f",
              ubxMessage.navPvt.fixType,
              ubxMessage.navPvt.numSV,
              ubxMessage.navPvt.lat * 1e-7,
              ubxMessage.navPvt.lon * 1e-7,
              ubxMessage.navPvt.gSpeed * 0.001
            );
          }
      // ----------------------------------------------------------
      // WAIT FOR GPS → live satellite updates + auto transition
      // ----------------------------------------------------------
      if (getMode() == MODE_WAIT_SATS)
      {
        uint8_t sv = ubxMessage.navPvt.numSV;

        // Redraw only if satellite count changes
        if (sv != lastSV) {
          lastSV = sv;
          screen_request_partial(1,1); // numbers arew hard coded for now
        }

        // Move on when GPS is good enough
        if (ubxMessage.navPvt.fixType >= 3 && sv >= 10) {
          setMode(MODE_LOGGING);
          screen_request_redraw();
        }
      }
    }
  
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}


// ==================================================
// GPS message handling
//
// IMPORTANT:
// - NAV-PVT is authoritative (fix, speed, time, SV count)
// - NAV-DOP is optional diagnostics only
// - NAV-SAT handles satellite display/logging
// ==================================================
static void processGpsMessages()
{
  // ---------------------------------------------------------------------------
  // NAV-PVT (authoritative navigation solution)
  // ---------------------------------------------------------------------------
  if (msgType == MT_NAV_PVT) {

    last_gps_msg = millis();

    if (Time_Set_OK) {
      nav_pvt_message++;
    }

    // -------- First fix detection --------
    if (!GPS_Signal_OK &&
        ubxMessage.navPvt.numSV >= MIN_numSV_FIRST_FIX &&
        (ubxMessage.navPvt.sAcc / 1000.0f) < MAX_Sacc_FIRST_FIX &&
        ubxMessage.navPvt.valid >= 7) {

      GPS_Signal_OK = true;
      first_fix_GPS = millis() / 1000;
    }

    // -------- Mode transition (idempotent) --------
    if (GPS_Signal_OK && getMode() == MODE_WAIT_SATS) {
      setMode(MODE_LOGGING);
    }

    if (GPS_Signal_OK) {
      GPS_delay++;
    }

    // -------- Time set & start logging --------
    if (!Time_Set_OK &&
        GPS_Signal_OK &&
        GPS_delay > (TIME_DELAY_FIRST_FIX * config.sample_rate)) {

      static int avg_speed = 0;
      avg_speed = (avg_speed + ubxMessage.navPvt.gSpeed * 19) / 20;

      if (avg_speed > (config.start_logging_speed * 1000)) {
        if (Set_GPS_Time(config.timezone)) {
          Time_Set_OK = true;
          Shut_down_Save_session = true;
          start_logging_millis = millis();
          Open_files();
        }
      }
    }

    // -------- Normal logging --------
    if ((sdOK || LITTLEFS_OK) &&
        Time_Set_OK &&
        nav_pvt_message > 10 &&
        nav_pvt_message != old_message) {

      old_message = nav_pvt_message;
      gps_speed_value   = ubxMessage.navPvt.gSpeed;

      static uint32_t last_flush_time = 0;
      if ((millis() - last_flush_time) > 60000UL) {
        Flush_files();
        last_flush_time = millis();
      }

      // Quality gate
      if (ubxMessage.navPvt.numSV <= MIN_numSV_GPS_SPEED_OK ||
          (ubxMessage.navPvt.sAcc / 1000.0f) > MAX_Sacc_GPS_SPEED_OK ||
          (ubxMessage.navPvt.gSpeed / 1000.0f) > MAX_GPS_SPEED_OK) {
        gps_speed_value = 0;
      }

      Log_to_SD();

      Ublox.push_data(
        ubxMessage.navPvt.lat / 10000000.0f,
        ubxMessage.navPvt.lon / 10000000.0f,
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

      if (run_count != old_run_count) {
        Ublox.run_distance = 0;
      }
      old_run_count = run_count;

      // Distance
      M100.Update_distance(run_count);
      M250.Update_distance(run_count);
      M500.Update_distance(run_count);
      M1852.Update_distance(run_count);

      // Speed
      S2.Update_speed(run_count);
      s2.Update_speed(run_count);
      S10.Update_speed(run_count);
      s10.Update_speed(run_count);
      S1800.Update_speed(run_count);
      S3600.Update_speed(run_count);

      // Alfa
      A250.Update_Alfa(M250);
      A500.Update_Alfa(M500);
      a500.Update_Alfa(M500);

      M_500.Update_Track();

      if (S2.avg_s > 4000) {
        S10_previous_run = S10.s_max_speed;
      }
    }

    return;
  }

  // ---------------------------------------------------------------------------
  // NAV-SAT (satellite info)
  // ---------------------------------------------------------------------------
  if (msgType == MT_NAV_SAT) {

    nav_sat_message++;

    if (ubxMessage.navSatCount > UBX_MAX_SVS) {
      ubxMessage.navSatCount = UBX_MAX_SVS;
    }

    Ublox_Sat.push_SAT_info(
      ubxMessage.navSatHdr,
      ubxMessage.navSat,
      ubxMessage.navSatCount
    );

    return;
  }

  // ---------------------------------------------------------------------------
  // NAV-DOP (optional diagnostics only)
  // ---------------------------------------------------------------------------
  if (msgType == MT_NAV_DOP) {
    // kept for compatibility / stats if needed later
    return;
  }
}
