#include "Display/screen_speed.h"
#include "Display/E_paper.h"      
#include "Fonts.h"      
#include "Display/screen_context.h"  
#include "GPS/GPS_data.h"
#include "Definitions.h"
#include "Layout.h"
#include "config_manager.h"
#include "Storage/storage_manager.h"
#include "Globals.h"

static int ui_offset = 0;

static char bar_info[16];

// -----------------------------------------------------------------------------
// Speed screen UI state (owned by screen_speed)
// -----------------------------------------------------------------------------
int bar_length = 1852;
int bar_position = 32;
int total_bar_length = 240;
int run_rectangle_length = 0;

void Speed_font0(const char*, const char*, float, float, float, int);
void Speed_font1(const char*, const char*, float, float, float, int);
void Speed_font3(const char*, float);

  void Speed_font0(
    const char* message1,
    const char* message2,
    float speed1,
    float speed2,
    float speed,
    int screen
) {
    const int decimals_small = (screen == 2) ? 0 : 1;

    display.setFont(Fonts::Body12);
    display.setTextColor(GxEPD_BLACK);

    display.setCursor(ui_offset, Layout::ROW12(1));
    display.print(message1);

    if (screen <= 2) {
        display.setFont(Fonts::Body18);
        display.print(speed1, decimals_small);
    }

    display.setFont(Fonts::Body12);
    display.setCursor(ui_offset + 122, Layout::ROW12(1));
    display.print(message2);

    display.setFont(Fonts::Body18);
    display.print(speed2, decimals_small);

    display.setFont(Fonts::SpeedXL);
    display.setCursor(ui_offset, 120);
    display.print(speed, 1);
}


void Speed_font1(
    const char* message1,
    const char* message2,
    float speed1,
    float speed2,
    float speed,
    int screen
) {
    display.setCursor(ui_offset, 36);

    if (screen == 0) {
        display.setFont(Fonts::SpeedM);
        display.print(speed1, 1);

        display.setFont(Fonts::Body12);
        display.setCursor(ui_offset + 113, 36);
        display.print(message2);

        display.setFont(Fonts::SpeedM);
        display.print(speed2, 1);
    }
    else if (screen == 1) {
        display.setFont(Fonts::Body12);
        display.print(message1);

        display.setFont(Fonts::SpeedM);
        display.print(speed1, 0);

        display.setFont(Fonts::Body12);
        display.print(message2);

        display.setFont(Fonts::SpeedM);
        display.print(speed2, 0);
    }
    else if (screen == 2) {
        display.setFont(Fonts::Body18);
        display.print(message1);

        display.setFont(Fonts::SpeedM);
        display.print(speed1, 2);
    }
    else if (screen == 3) {
        display.setFont(Fonts::Body18);
        display.print(message1);
    }

    display.setFont(Fonts::SpeedXL);
    display.setCursor(ui_offset, 120);
    display.println(speed, 1);
}

void Speed_font3(
    const char* message1,
    float speed
) {
    display.setFont(&FreeSansBold24pt7b);
    display.setCursor(ui_offset, 36);
    bar_position = 40;
    display.print(message1);

    display.setCursor(ui_offset, 120);
    display.setFont(Fonts::SpeedXL);
    display.print(speed, 1);
}

void draw_SPEED() 
{

    int field = config.field_actual;
    bool alfa_screen =
      (Ublox.alfa_distance / 1000 < 350) && (abs(alfa_window) < 100);
    bool nautical_mile_screen =
      (Ublox.alfa_distance / 1000 > 1852);
    bool x_10km_screen =
      ((int)(Ublox.total_distance / 1000000) % 10 == 0) &&
      (Ublox.alfa_distance / 1000 > 1000);

    display.setFont(Fonts::Small6);
    display.setCursor(display.width() - 20, 0);
    display.print((char)config.field_actual);

    switch (config.field_actual) {
      case SPEED1:
        field = alfa_screen ? SPEED3 :
                nautical_mile_screen ? SPEED4 :
                x_10km_screen ? SPEED5 : SPEED2;
        break;

      case SPEED2:
        field = nautical_mile_screen ? SPEED4 : SPEED2;
        break;

      case SPEED7:
      case SPEED8:
        field = alfa_screen ? SPEED3 : config.field_actual;
        break;

      case SPEED9:
        field = SPEED2;
        if (nautical_mile_screen) field = SPEED4;
        if (Ublox.alfa_distance / 1000 < 1000) field = SPEED8;
        if (S10.s_max_speed > S10.display_speed[5]) field = SPEED2;
        if (alfa_screen) field = SPEED3;
        break;
    }

    if (GPS_Signal_OK) {
      if (config.speed_large_font == 2) {
        int komma = int(gps_speed_value * calibration_speed * 10) % 10;
        display.setFont(Fonts::Huge75);
        display.setCursor(ui_offset - 6, 115);
        display.print(int(gps_speed_value * calibration_speed));
        display.setFont(Fonts::Big30);  display.print(".");
        display.setFont(Fonts::SpeedL); display.println(komma);
      }
    } else {
      display.setFont(Fonts::Body18);
      display.setCursor(ui_offset, 60);
      display.print("Low GPS signal !");
    }

    if (field <= SPEED2) {
      float run = S10.display_last_run * calibration_speed;
      float avg = S10.avg_5runs * calibration_speed;
      float cur = gps_speed_value * calibration_speed;

      if (config.speed_large_font == 0)
        Speed_font0("Run", "Avg ", run, avg, cur, 0);
      else if (config.speed_large_font == 1)
        Speed_font1("", "A", run, avg, cur, 0);
      else if (config.speed_large_font == 3)
        Speed_font3(S10.s_max_speed < S10.display_speed[5] ? "   SPEED" : "LAST RUN",
                    (S10.s_max_speed < S10.display_speed[5]) ? cur
                                                             : S10.s_max_speed * calibration_speed);
    }

    /*
      First 250m after jibe, if Window>99 m : Window and Exit
      Between 250m and 400m after jibe : Result Alfa (speed or MISS)
      Between 400m and 1852m after jibe : Actual Run + AVG
      More then 1852m : NM actual speed and NM Best speed
      */
      if (field == SPEED3) {
      bool gate = (abs(alfa_window) < 99) && (Ublox.alfa_distance / 1000 < 255);
      float cur = gps_speed_value * calibration_speed;
      float alfa = A500.alfa_speed_max * calibration_speed;

      if (gate && alfa_exit > 99) alfa_exit = 99;

      if (config.speed_large_font == 0) {
        if (gate)
          Speed_font0("Gate", " Ex ", alfa_window, alfa_exit, cur, 2);
        else if (alfa > 1)
          Speed_font0("Alfa ", "Ab ", A500.display_max_speed * calibration_speed, alfa, cur, 1);
        else
          Speed_font0("Alfa MISS", "Ab ", 0, alfa, cur, 3);
      }

      else if (config.speed_large_font == 1) {
        if (gate)
          Speed_font1("Gate", "Ex", alfa_window, alfa_exit, cur, 1);
        else if (alfa > 1)
          Speed_font1("Alfa= ", "", alfa, 0, cur, 2);
        else
          Speed_font1("Alfa = MISS", "", 0, 0, cur, 3);
      }

      else if (config.speed_large_font == 3) {
        if (gate)
          Speed_font3("   GATE", alfa_window);
        else if (alfa > 1)
          Speed_font3("   Alfa", alfa);
        else
          Speed_font3("Alfa MISS", -1);
      }
    }

    float cur = gps_speed_value * calibration_speed;

    if (field == SPEED4) {
      float nm = M1852.m_max_speed * calibration_speed;
      if (config.speed_large_font == 0)
        Speed_font0("NMa ", " NM ", M1852.display_speed[9] * calibration_speed, nm, cur, 0);
      else if (config.speed_large_font == 1)
        Speed_font1("NM= ", " ", nm, 0, cur, 2);
      else if (config.speed_large_font == 3)
        Speed_font3("  N. MILE", nm);
    }

    if (field == SPEED5) {
      float dist_km = Ublox.total_distance / 1000000.0;
      if (config.speed_large_font == 1)
        Speed_font1("Dist ", " ", dist_km, 0, cur, 2);
      else if (config.speed_large_font == 3)
        Speed_font3("Dist ", dist_km);
      else if (config.speed_large_font == 0)
        Speed_font0("Run", " Dist",
                    Ublox.total_distance / 1000.0,
                    Ublox.alfa_distance / 1000000.0,
                    cur, 2);
    }

    if (field == SPEED6 && config.speed_large_font != 2 && config.speed_large_font != 4)
      Speed_font0("2S ", "10S ",
                  S2.display_max_speed * calibration_speed,
                  S10.display_max_speed * calibration_speed,
                  cur, 1);

    if (field == SPEED7) {
      float m500 = M500.m_max_speed * calibration_speed;
      if (config.speed_large_font == 1)
        Speed_font1("500m ", " ", m500, m500, cur, 2);
      else if (config.speed_large_font == 3)
        Speed_font3("500m :", m500);
      else if (config.speed_large_font == 0)
        Speed_font0("500A", "Max", m500,
                    M500.display_speed[9] * calibration_speed,
                    cur, 1);
    }


     if (config.speed_large_font == 4) {
      double speed = gps_speed_value * calibration_speed;
      display.setFont(Fonts::Huge75);
      display.setCursor(ui_offset - 6, 118);
      display.print(speed, 0);
      display.setFont(Fonts::SpeedL);
      display.print(".");
      display.print(int(speed * 10) % 10);
    }

    static int low_speed_seconds, start_hour_millis = millis();
    int log_seconds = (millis() - start_hour_millis) / 1000;

    if (S10.avg_s > 2000) low_speed_seconds = 0;
    if (++low_speed_seconds > 120) start_hour_millis = millis();

    if (field == SPEED8) {
      if (log_seconds > 3600) start_hour_millis = millis();

      float max1h = S3600.display_max_speed * calibration_speed;
      float avg1h = S3600.avg_s * calibration_speed;
      float cur   = gps_speed_value * calibration_speed;

      if (config.speed_large_font == 1)
        Speed_font1("1h: ", " ", max1h, avg1h, cur, 2);
      else if (config.speed_large_font == 3)
        Speed_font3("1 Hour", max1h);
      else if (config.speed_large_font == 0)
        Speed_font0("1hA ", "1hB ", avg1h, max1h, cur, 0);
    }



    if (field == SPEEDA && config.speed_large_font != 2 && config.speed_large_font != 4)
      Speed_font0("CM ", "TM ",
                  S2.display_last_run * calibration_speed,
                  S2.display_max_speed * calibration_speed,
                  gps_speed_value * calibration_speed, 1);

    // progress bar -------------------------------------------------
    if (config.speed_large_font == 0)      { total_bar_length = 180; bar_position = 32; }
    else if (config.speed_large_font == 1) { total_bar_length = 240; bar_position = 38; }
    else                                   { total_bar_length = 240; bar_position = 0; }

    bar_length = config.bar_length * 1000 / total_bar_length;
    sprintf(bar_info, "%dm", config.bar_length);

    if (field == SPEED3) {
      bar_length = 250 * 1000 / total_bar_length;
      strcpy(bar_info, "250m");
    }

    if (field == SPEED7) {
      bar_length = 500 * 1000 / total_bar_length;
      strcpy(bar_info, "500m");
    }

    if (field == SPEED8) {
      run_rectangle_length = log_seconds * total_bar_length / 3600;
      strcpy(bar_info, "3600s");
    } else {
      run_rectangle_length = Ublox.alfa_distance / bar_length;
    }

    if (run_rectangle_length > total_bar_length)
      run_rectangle_length = total_bar_length;

    if (config.speed_large_font == 1 && run_rectangle_length < 180) {
      display.setTextWrap(false);
      display.setFont(Fonts::Mono9);
      display.setCursor(ui_offset + 186, 48);
      display.print(bar_info);
    }

    if (config.speed_large_font == 0) {
      display.setFont(Fonts::Body12);
      display.setCursor(ui_offset + 180, 43);
      display.print(time_now);
    }

    if (config.speed_large_font == 2 && run_rectangle_length < 160) {
      display.setTextWrap(false);
      display.setFont(Fonts::Mono9);
      display.setCursor(ui_offset + 186, 10);
      display.print(bar_info);
    }

    display.fillRect(ui_offset, bar_position, run_rectangle_length, 8, GxEPD_BLACK);
  
}
