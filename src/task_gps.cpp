#include <Arduino.h>
#include "Globals.h"

#include "task_gps.h"
#include "config_manager.h"
#include "storage_manager.h"
#include "system_mode.h"

#include "Ublox.h"
#include "SD_card.h"
#include "ESP_functions.h"
#include "E_paper.h"

#include <SD_MMC.h>
#include "Definitions.h"

int GPS_delay        = 0;

// --------------------------------------------------
// Task handle
// --------------------------------------------------
TaskHandle_t t1 = nullptr;

// --------------------------------------------------
// External state
// --------------------------------------------------
extern bool sleep_mode;

// --------------------------------------------------
// Local helpers
// --------------------------------------------------
static void processGpsMessages();

// --------------------------------------------------
// GPS task
// --------------------------------------------------
void taskOne(void *parameter)
{
  static int actual_speed_field = 0;

  for (;;) {

    // ----------------------------------------------
    // Mode gate — GPS only runs in field modes
    // ----------------------------------------------
    SystemMode mode = getMode();
    if (mode != MODE_LOGGING && mode != MODE_FIELD_CONFIG) {
      vTaskDelay(pdMS_TO_TICKS(500));
      continue;
    }

    wdt_task0 = millis();



  
    // ----------------------------------------------
    // GPS processing
    // ----------------------------------------------
#if defined(STATIC_DEBUG)
    msgType = processGPS();

    static int testtime = 0;
    if ((millis() - testtime) > 1000) {
      testtime = millis();
      Set_GPS_Time(config.timezone);
      config.timezone++;
    }
#else
    msgType = processGPS();

 
#endif

    // ----------------------------------------------
    // Message handling
    // ----------------------------------------------
    processGpsMessages();
  }
}




// ==================================================
// GPS message handling
// ==================================================
static void processGpsMessages()
{
  if (msgType == MT_NAV_DOP) {

    // -------- First fix detection --------
    if (!GPS_Signal_OK &&
        ubxMessage.navPvt.numSV >= MIN_numSV_FIRST_FIX &&
        (ubxMessage.navPvt.sAcc / 1000.0f) < MAX_Sacc_FIRST_FIX &&
        ubxMessage.navPvt.valid >= 7) {

      GPS_Signal_OK = true;
      first_fix_GPS = millis() / 1000;
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
      gps_speed = ubxMessage.navPvt.gSpeed;

      static int last_flush_time = 0;
      if ((millis() - last_flush_time) > 60000) {
        Flush_files();
        last_flush_time = millis();
      }

      if (ubxMessage.navPvt.numSV <= MIN_numSV_GPS_SPEED_OK ||
          (ubxMessage.navPvt.sAcc / 1000.0f) > MAX_Sacc_GPS_SPEED_OK ||
          (ubxMessage.navPvt.gSpeed / 1000.0f) > MAX_GPS_SPEED_OK) {
        gps_speed = 0;
      }

      Log_to_SD();

      Ublox.push_data(
        ubxMessage.navPvt.lat / 10000000.0f,
        ubxMessage.navPvt.lon / 10000000.0f,
        gps_speed
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
  }
  else if (msgType == MT_NAV_PVT) {
    last_gps_msg = millis();
    if (Time_Set_OK) {
      nav_pvt_message++;
    }
  }
  else if (msgType == MT_NAV_SAT) {
    nav_sat_message++;
    Ublox_Sat.push_SAT_info(ubxMessage.navSat);
  }
}
