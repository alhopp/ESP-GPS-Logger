#include "task_gps.h"
#include "config_manager.h"

TaskHandle_t t1 = nullptr;


// bring in exactly what taskOne already relied on
#include "wifi_manager.h"
#include "Ublox.h"
#include "SD_card.h"
#include "ESP_functions.h"
#include "E_paper.h"

#include <SD_MMC.h>

extern bool sleep_mode;

void taskOne(void *parameter)
{
    
  Serial.println("[TASK1] GPS task disabled (stub)");

  // TESTING TESTING Just idle forever 
  while (true) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  static int actual_speed_field = 0;

  while (true) {
    wdt_task0 = millis();

    // -------------------------------------------------
    // GPIO buttons
    // -------------------------------------------------
#if defined(GPIO12_ACTIF)
    if (Short_push12.Button_pushed()) GPIO12_screen++;
    if (GPIO12_screen > config.gpio12_count) GPIO12_screen = 0;

    if (Long_push12.Button_pushed()) {
      s10.Reset_stats();
      s2.Reset_stats();
      a500.Reset_stats();
    }
#endif

    static bool buttons_enabled = false;
    if (buttons_enabled) {
        if (Long_push39.Button_pushed() | Long_push19.Button_pushed()) {
        sleep_mode = true;
        Serial.println("task one delete");
        vTaskDelete(NULL);
        }
    }   

    if (Short_push39.Button_pushed() | Short_push19.Button_pushed()) {
      if (config.Stat_screens_time == 0) stat_count++;
      else actual_speed_field++;
    }

    if (actual_speed_field > config.speed_count) actual_speed_field = 0;
    if (stat_count > config.screen_count) stat_count = 0;

    config.field_actual = config.speed_screen[actual_speed_field];
    Field_choice =
        ((Short_push39.long_pulse | Short_push19.long_pulse) &&
         (config.Stat_screens_time != 0));

    // -------------------------------------------------
    // WiFi handling (isolated)
    // -------------------------------------------------
    wifi_handle();

    // Create Archive dir if needed
    if (sdOK && !SD_MMC.exists("/Archive")) {
      SD_MMC.mkdir("/Archive");
    }

#if defined(USE_AUTO_OTA_UPDATE)
    if ((millis() - OTA_CHECK_INTERVAL) > _lastOTACheck) {
      _lastOTACheck = millis();
      checkFirmwareUpdates();
    }
#endif

    // -------------------------------------------------
    // GPS processing
    // -------------------------------------------------
#if defined(STATIC_DEBUG)
    msgType = processGPS();
    static int testtime;
    if ((millis() - testtime) > 1000) {
      testtime = millis();
      Set_GPS_Time(config.timezone);
      config.timezone++;
    }
#else
    msgType = processGPS();
    if ((millis() - last_gps_msg) > TIME_OUT_NAV_PVT)
      trouble_screen = true;
    else
      trouble_screen = false;
#endif

    // -------------------------------------------------
    // Message handling
    // -------------------------------------------------
    if (msgType == MT_NAV_DOP) {

      if ((ubxMessage.navPvt.numSV >= MIN_numSV_FIRST_FIX) &&
          ((ubxMessage.navPvt.sAcc / 1000.0f) < MAX_Sacc_FIRST_FIX) &&
          (ubxMessage.navPvt.valid >= 7) &&
          (GPS_Signal_OK == false)) {

        GPS_Signal_OK = true;
        first_fix_GPS = millis() / 1000;
      }

      if (GPS_Signal_OK) GPS_delay++;

      if (!Time_Set_OK && GPS_Signal_OK &&
          (GPS_delay > (TIME_DELAY_FIRST_FIX * config.sample_rate))) {

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

      if ((sdOK || LITTLEFS_OK) && Time_Set_OK &&
          (nav_pvt_message > 10) &&
          (nav_pvt_message != old_message)) {

        old_message = nav_pvt_message;
        gps_speed = ubxMessage.navPvt.gSpeed;

        static int last_flush_time = 0;
        if ((millis() - last_flush_time) > 60000) {
          Flush_files();
          last_flush_time = millis();
        }

        if ((ubxMessage.navPvt.numSV <= MIN_numSV_GPS_SPEED_OK) ||
            ((ubxMessage.navPvt.sAcc / 1000.0f) > MAX_Sacc_GPS_SPEED_OK) ||
            ((ubxMessage.navPvt.gSpeed / 1000.0f) > MAX_GPS_SPEED_OK)) {
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

        if (run_count != old_run_count) Ublox.run_distance = 0;
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

        if (S2.avg_s > 4000)
          S10_previous_run = S10.s_max_speed;
      }
    }
    else if (msgType == MT_NAV_PVT) {
      last_gps_msg = millis();
      if (Time_Set_OK) nav_pvt_message++;
    }
    else if (msgType == MT_NAV_SAT) {
      nav_sat_message++;
      Ublox_Sat.push_SAT_info(ubxMessage.navSat);
    }
  }
}