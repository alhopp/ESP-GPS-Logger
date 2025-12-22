
#include <Arduino.h>
#include "E_paper.h"
#include "Fonts.h"

#include "Ublox.h"
#include "GPS_data.h"

#include "Definitions.h"

#include "SD_card.h"
#include <LittleFS.h>

#include "screens.h"
#include "screen_draw.h"
#include "screen_speed.h"
#include "screen_ui.h"

#include "Layout.h"

#include "screen_system.h"
#include "config_manager.h"
#include "storage_manager.h"
#include "esp_logo.h"

GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(
  GxEPD2_213_B74(ELINK_SS, ELINK_DC, ELINK_RESET, ELINK_BUSY)
);

int16_t displayWidth  = 0;
int16_t displayHeight = 0;

static int update_epaper = 2;

// bottom area 15px reserved to info bar
#define INFO_BAR_HEIGHT 15
#define INFO_BAR_TOP (display.height() - INFO_BAR_HEIGHT)
#define INFO_BAR_ROW (display.height() - 2)

  // --- Chrome helpers (forward declarations) ---
void Bat_level_Simon(int offset);
void Sats_level(int offset);
void M8_M10(int offset);
int  Time(int offset);
int  DateTimeRtc(int offset);



void drawChrome(int offset, bool rtcMode)
{
  if (rtcMode) {
    Bat_level_Simon(offset);
    DateTimeRtc(offset);
  } else {
    Bat_level_Simon(offset);
    Sats_level(offset);
    if (ubxMessage.navPvt.numSV > 4)
      M8_M10(offset);
    Time(offset);
  }
}


#if defined(EPD_213_B74)
  const char E_paper_version[] = "E-paper 213B74";
#elif defined(EPD_213_B73)
  const char E_paper_version[] = "E-paper 213B73";
#elif defined(EPD_266_BN)
  const char E_paper_version[] = "E-paper 266BN";
#else
  const char E_paper_version[] = "E-paper unknown";
#endif

int device_boot_log(int rows, int ws)
{
  int r = 2;

  auto pause = [&]() {
    if (ws) delay(ws);
  };

  // --------------------------------------------------
  // Header: device + firmware
  // --------------------------------------------------
  display.setCursor(offset, Layout::ROW9(2));
  pause();
  display.print(E_paper_version);
  display.print(SW_version);

  // --------------------------------------------------
  // SD / filesystem info
  // --------------------------------------------------
  const bool show_storage =
      (rows == 2 || rows == 23 || rows == 24 || rows == 234);

  if (show_storage) {
    display.setCursor(offset, Layout::ROW9(3));
    pause();
    sdCardInfo();
  }

  // --------------------------------------------------
  // Cursor advance / spacing logic
  // --------------------------------------------------
  const bool advance_row =
      (rows == 3 || rows == 23 || rows == 34 || rows == 234);

  if (advance_row) {
    r = (rows == 234) ? 4
        : (rows == 23 || rows == 34) ? 3
        : 2;

    display.setCursor(
      offset,
      (rows == 234 || rows == 23 || rows == 34)
        ? Layout::ROW9(4)
        : Layout::ROW9(3)
    );
  }

  // --------------------------------------------------
  // GPS info
  // --------------------------------------------------
  const bool show_gps =
      (rows == 4 || rows == 24 || rows == 34 || rows == 234) &&
      ubxMessage.monVER.hwVersion[0];

  if (show_gps) {
    display.setCursor(
      offset,
      (rows == 234) ? Layout::ROW9(5)
      : (rows == 24 || rows == 34) ? Layout::ROW9(4)
      : Layout::ROW9(3)
    );

    display.printf("Gps %s at %dHz",
                    gpsChip(1),
                    config.sample_rate);
  }

  return r;
}


#ifndef T5_E_PAPER

#else

#define device_boot_log(rows) device_boot_log(rows, 0)

char time_now[8];
char time_now_sec[12];


int bar_length = 1852;
int bar_position = 32;
int total_bar_length = 240;
int run_rectangle_length = 0;
void InfoBar(int offset);
void InfoBarRtc(int offset);
char bar_info[8] = "info";



#ifdef TRACKSPEED
static void draw_STATSC();
static void draw_STATSD();
#endif





int update_time() {
  int ret = 0;
  if (!NTP_time_set) {
    if (!Gps_time_set) {
      if (Set_GPS_Time(config.timezone)) Gps_time_set = 1;
    }
  }
  if ((!Gps_time_set && !NTP_time_set) || !getLocalTime(&tmstruct)) return 1;
  sprintf(time_now, "%02d:%02d", tmstruct.tm_hour, tmstruct.tm_min);
  sprintf(time_now_sec, "%02d:%02d:%02d", tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);
  return ret;
}

//for print hour&minutes with 2 digits
void time_print(int time) {
  if (time < 10) display.print("0");
  display.print(time);
}
void Bat_level(int X_offset, int Y_offset) {
  float bat_symbol = 0;
  display.fillRect(X_offset + 3, Y_offset, 6, 3, GxEPD_BLACK);
  display.fillRect(X_offset, Y_offset + 3, 12, 30, GxEPD_BLACK);  //monitor=(4.2-RTC_voltage_bat)*26
  if (RTC_voltage_bat < VOLTAGE_100) {
    bat_symbol = (VOLTAGE_100 - RTC_voltage_bat) * 28;
    display.fillRect(X_offset + 2, Y_offset + 7, 8, (int)bat_symbol, GxEPD_WHITE);
  }
}


void Bat_level_Simon(int offset) {
  float bat_perc = 100 * (1 - (VOLTAGE_100 - RTC_voltage_bat) / (VOLTAGE_100 - VOLTAGE_0));
  if (bat_perc < 0) bat_perc = 0;
  if (bat_perc > 100) bat_perc = 100;

  int batW = 8;
  int batL = 15;
  int posX = display.width() - batW - 6;  //was -10
  int posY = display.height() - batL;
  int line = 2;
  int seg = 3;
  int segW = batW - 2 * line;
  int segL = (batL - 0.25 * batW - 2 * line - (seg - 1)) / seg;
  display.fillRect(offset + posX, posY, 0.5 * batW, 0.25 * batW, GxEPD_BLACK);                 //battery top
  display.fillRect(offset + posX - 0.25 * batW, posY + 0.25 * batW, batW, batL, GxEPD_BLACK);  //battery body
  if (bat_perc < 67) display.fillRect(offset + posX - 0.25 * batW + line, posY + 0.25 * batW + line, segW, segL, GxEPD_WHITE);
  if (bat_perc < 33) display.fillRect(offset + posX - 0.25 * batW + line, posY + 0.25 * batW + line + 1 * (segL + 1), segW, segL, GxEPD_WHITE);
  if (bat_perc < 1) display.fillRect(offset + posX - 0.25 * batW + line, posY + 0.25 * batW + line + 2 * (segL + 1), segW, segL, GxEPD_WHITE);
  //Serial.printf("info bar cursor pos: %d, display height: %d\n", INFO_BAR_ROW,display.height());
  display.setFont(Fonts::Body9);
  //display.setCursor(display.width()-8,(INFO_BAR_ROW-ROW_9PT));
  //display.print("-");
  if (bat_perc < 100) display.setCursor(offset + 156, (INFO_BAR_ROW));  //was 193
  else display.setCursor(offset + 146, (INFO_BAR_ROW));                 //was 184
  display.print(RTC_voltage_bat + 0.04, 1);
  display.print("V ");
  display.print(int(bat_perc));
  display.print("%");
}


void Sats_level(int offset) {
  if (!ubxMessage.monVER.swVersion[0]) return;
  // int circelL = 5;
  //int circelS = 2;
  int posX = 120 + offset;  //was 176
  int posY = INFO_BAR_TOP;  //-(circelL+2*circelS);
  int satnum = ubxMessage.navPvt.numSV;
  //display.drawExampleBitmap(ESP_Sat_15, posX, posY, 15, 15, GxEPD_BLACK);
  display.setFont(Fonts::Body9);
  display.setCursor(posX - (satnum < 10 ? 9 : 18), INFO_BAR_ROW);
  display.print(satnum);
}

void M8_M10(int offset) {
  display.setFont(Fonts::Body9);
  display.setCursor(offset + 60, INFO_BAR_ROW);
  display.print(gpsChip(0));
}


int Time(int offset) {
  if (!update_time()) {
    display.setFont(Fonts::Body9);
    display.setCursor(offset, INFO_BAR_ROW);
    display.print(time_now);
  }
  return 0;
}
int TimeRtc(int offset) {
  display.setFont(Fonts::Body9);
  display.setCursor(offset, INFO_BAR_ROW);
  display.printf("%d:%d", RTC_hour, RTC_min);
  return 0;
}


int DateTimeRtc(int offset) {
  display.setFont(Fonts::Body9);
  display.setCursor(offset, INFO_BAR_ROW);
  display.printf("%02d:%02d %02d-%02d-%02d", RTC_hour, RTC_min, RTC_day, RTC_month, RTC_year);
  return 0;
}


void InfoBar(int offset) {
  Bat_level_Simon(offset);
  Sats_level(offset);
  if (ubxMessage.navPvt.numSV > 4) M8_M10(offset);
  Time(offset);
}

void InfoBarRtc(int offset) {
  Bat_level_Simon(offset);
  DateTimeRtc(offset);
}

void Speed_in_Unit(int offset) {
  display.setRotation(0);
  display.setFont(Fonts::Small6);
  display.setCursor(30, offset + 245);                                            //was 30, 249
  if ((int)(calibration_speed * 100000) == 194) display.print("speed in knots");  //1.94384449 m/s to knots !!!
  if ((int)(calibration_speed * 1000000) == 3600) display.print("speed in km/h");
  display.setRotation(1);
}

void sdCardInfo()
{
  if (sdOK) {
    uint64_t free_mb = storageFreeKBytes() / 1024;
    display.printf("SD    : %llu MB\n", free_mb);
  }
  else if (LITTLEFS_OK) {
    uint64_t free_kb = storageFreeKBytes();
    display.printf("Local : %llu KB\n", free_kb);
  }
}


static void printValueSmart(float v) {
  if (v < 100.0f)      display.println(v, 2);
  else if (v < 1000.0f) display.println(v, 1);
  else                 display.println(v, 0);
}











void Update_screen(int screen)
{
  static int count = 0;
  static int old_screen = -1;
  static int update_delay = 0;

  update_time();
  update_epaper = 1;

  // subtle horizontal wobble
  offset += ((count / 4) % 20 < 10) ? 1 : -1;
  if (offset < 0 || offset > 10) offset = 0;

  int cursor = 0;

  // allow screen to prepare data
  ScreenDrawTable[screen]();

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    // Chrome everywhere except pure speed/stats
    if (screen != SPEED &&
        screen != STATS9 &&
        screen != STATS8 &&
        screen != STATSA &&
        screen != STATSD) {
      drawChrome(offset, false);
    }

    switch (screen) {

      // --------------------------------------------------
      case BOOT_SCREEN:
        update_delay = 1000;
  
        drawTopLeftTitle("ESP-GPS config");
        device_boot_log(234);
        Speed_in_Unit(offset);
        if (screen != old_screen) count = 0;
        break;

      // --------------------------------------------------
      case GPS_INIT_SCREEN:
        update_delay = 100;

        drawTopLeftTitle("ESP-GPS GPS init");
        device_boot_log(24);

        display.setFont(Fonts::Body12);

        if (config.ublox_type == 0xFF) {
          display.setCursor(offset,
            cursor = Layout::ROW9(4) + Layout::STEP12);
          display.print("Auto detect gps-type");
        }
        else if (!ubxMessage.monVER.hwVersion[0]) {
          display.setCursor(offset,
            cursor = Layout::ROW9(3) + Layout::STEP12);
          display.println("Gps initializing");

          if (config.M10_high_nav == M10_HIGH_NAV_RATE)
            display.println("M10 high nav mode !");
          if (config.M10_high_nav == M10_DEFAULT_NAV)
            display.println("M10 default nav mode");
        }

        Speed_in_Unit(offset);
        if (screen != old_screen) count = 0;
        break;

      default:
        break;
    }

  } while (display.nextPage());

  if (screen == BOOT_SCREEN)
    delay(1000);

  // ==================================================
  // WIFI SCREENS (not paged)
  // ==================================================

  if (screen == WIFI_ON) {
    update_delay = 100;
    offset += (count % 20 < 10) ? 1 : -1;


    drawTopLeftTitle("ESP-GPS connect");
    device_boot_log(2);

    if (!SoftAP_connection) {
      display.setCursor(offset, 102);
      display.printf("Logspace left : %d hour",
        storageLogTimeLeftMinutes() / 60);
    }

    if (Wifi_on) {
      display.setFont(Fonts::Body12);
      display.setCursor(offset,
        cursor = Layout::ROW9(3) + Layout::STEP12);
      display.print("Ssid: ");
      display.print(SoftAP_connection ? "ESP32AP" : actual_ssid);

      display.setFont(Fonts::Body9);
      display.setCursor(offset, cursor += Layout::STEP9);

      if (SoftAP_connection) {
        display.print("Password: password");
        display.setCursor(offset, cursor += Layout::STEP9);
      }

      display.printf("http://%s", IP_adress.c_str());
    }
    else {
      display.fillRect(0, 0, 250, 122, GxEPD_WHITE);


      drawTopLeftTitle("ESP-GPS ready");
      device_boot_log(24);

      display.setFont(Fonts::Body12);
      display.setCursor(offset,
        cursor = Layout::ROW9(4) + Layout::STEP12);

      if (ubxMessage.navPvt.numSV < 5) {
        display.println("Waiting for Sat >=5");
        display.setFont(Fonts::Body9);
        display.setCursor(offset, 102);
        display.println("Please go outside...");
      }
      else {
        display.println("Ready for action");
        display.setFont(Fonts::Body9);
        display.setCursor(offset, cursor += Layout::STEP9);
        display.print("Move faster than ");

        if ((int)(calibration_speed * 100000) == 194)
          display.print(config.start_logging_speed * 1.94384449), display.print("kn");
        if ((int)(calibration_speed * 1000000) == 3600)
          display.print(config.start_logging_speed * 3.6), display.print("km/h");

        Speed_in_Unit(offset);
      }

      drawChrome(offset, false);
    }
  }

  else if (screen == WIFI_STATION) {
    update_delay = 100;

    drawTopLeftTitle("ESP-GPS try to connect");
    device_boot_log(2);

    display.setCursor(offset, 102);
    display.printf("Logspace left : %d hour",
      Logtime_left(storageLogTimeLeftMinutes()) / 60);

    display.setFont(Fonts::Body12);
    display.setCursor(offset,
      cursor = Layout::ROW9(3) + Layout::STEP12);
    display.print(actual_ssid);

    display.setFont(Fonts::Body9);
    display.setCursor(offset, cursor += Layout::STEP9);
    display.printf("For AP: use magnet in %ds", wifi_search);

    if (screen != old_screen) count = 0;
  }

  else if (screen == WIFI_SOFT_AP) {
    update_delay = 100;


    drawTopLeftTitle("Connect to ESP-GPS");
    device_boot_log(2);

    display.setFont(Fonts::Body12);
    display.setCursor(offset,
      cursor = Layout::ROW9(3) + Layout::STEP12);
    display.print("Ssid: ESP32AP");

    display.setFont(Fonts::Body9);
    display.setCursor(offset, cursor += Layout::STEP9);
    display.print("Password: password");

    display.setCursor(offset, cursor += Layout::STEP9);
    display.printf("http://%s/ in %ds",
      IP_adress.c_str(), wifi_search);

    if (screen != old_screen) count = 0;
  }

  else if (screen == TROUBLE) {
    display.setFont(Fonts::Body12);
    display.setCursor(offset, Layout::ROW12(1));
    display.println("No GPS frames for");
    display.println("more then 10 s....");
  }

  old_screen = screen;
  count++;



    #ifdef TRACKSPEED
        if (screen == STATSC)
          Stats_4lines("Dis_S:", "Dis:", "Speed:", "Dis_E:",
                      M_500.distance_startline,
                      M_500.track_distance,
                      M_500.Track_speed,
                      M_500.distance_endline);

        if (screen == STATSD) {
          display.setFont(Fonts::Body12);
          for (int i = 9; i > 4; i--) {
            int y = 24 * (10 - i);
            display.setCursor(offset, y);
            display.print("Track"); display.print(10 - i); display.print(": ");
            display.print(M_500.avg_speed[i] * config.cal_speed, 2);
            display.print(" @");
            display.print(M_500.time_hour[i]);
            display.print(M_500.time_min[i] < 10 ? ":0" : ":");
            display.print(M_500.time_min[i]);
          }
        }
      #endif
#endif

}

