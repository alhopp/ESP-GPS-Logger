#include <Arduino.h>

#include "E_paper.h"
#include "GxEPD.h"
#include "Fonts.h"

#include "Ublox.h"
#include "GPS_data.h"
#include "Definitions.h"
#include <LittleFS.h>

// Pin mapping – adjust per board later
#define EPD_CS   5
#define EPD_DC   17
#define EPD_RST  16
#define EPD_BUSY 4
GxEPD_Class display(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);


static int update_epaper = 2;

namespace Layout {

  // global vertical spacing between rows
  constexpr int SPACING = 2;

  // base row heights
  constexpr int H9  = 14;
  constexpr int H12 = 19;
  constexpr int H18 = 25;

  // absolute row positions (top-aligned)
  constexpr int ROW9(int n)  { return H9  + (n - 1) * (H9  + SPACING); }
  constexpr int ROW12(int n) { return H12 + (n - 1) * (H12 + SPACING); }
  constexpr int ROW18(int n) { return H18 + (n - 1) * (H18 + SPACING); }

  // row-to-row spacing helpers (replacement for *_W_SPACING)
  constexpr int STEP9  = H9  + SPACING;
  constexpr int STEP12 = H12 + SPACING;
  constexpr int STEP18 = H18 + SPACING;

}

namespace Fonts {

  constexpr const GFXfont* Small6  = &FreeSansBold6pt7b;

  constexpr const GFXfont* Body9   = &FreeSansBold9pt7b;
  constexpr const GFXfont* Body12  = &FreeSansBold12pt7b;
  constexpr const GFXfont* Body18  = &FreeSansBold18pt7b;

  constexpr const GFXfont* Mono9   = &FreeMonoBold9pt7b;
  constexpr const GFXfont* Mono12  = &FreeMonoBold12pt7b;

  constexpr const GFXfont* SpeedM  = &SansSerif_bold_46_nr;
  constexpr const GFXfont* SpeedL  = &SansSerif_bold_84_nr;
  constexpr const GFXfont* SpeedXL = &SansSerif_bold_96_nr;

  constexpr const GFXfont* Big30   = &FreeSansBold30pt7b;
  constexpr const GFXfont* Huge75  = &FreeSansBold75pt7b;

}


// bottom area 15px reserved to info bar
#define INFO_BAR_HEIGHT 15
#define INFO_BAR_TOP (displayHeight - INFO_BAR_HEIGHT)
#define INFO_BAR_ROW (displayHeight - 2)

//display.setFont(Fonts::Body9);
#define TITLE_9PT \
  do { \
    display.setFont(Fonts::Body9); \
    display.setCursor(offset, Layout::ROW9(1)); \
  } while (0)

#define TOP_TITLE_MSG(msg) \
  do { \
    display.print(msg); \
  } while (0)

#define TOP_LEFT_TITLE_MSG(msg) \
  do { \
    TITLE_9PT; \
    TOP_TITLE_MSG(msg); \
  } while (0)


#if defined(EPD_213_B74)
  const char E_paper_version[] = "E-paper 213B74";
#elif defined(EPD_213_B73)
  const char E_paper_version[] = "E-paper 213B73";
#elif defined(EPD_266_BN)
  const char E_paper_version[] = "E-paper 266BN";
#else
  const char E_paper_version[] = "E-paper unknown";
#endif


#ifndef T5_E_PAPER
void Boot_screen(void){};
void Sleep_screen(int choice){};
void Update_screen(int screen){};
#else
char time_now[8];
char time_now_sec[12];
int16_t displayHeight;
int16_t displayWidth;
int bar_length = 1852;
int bar_position = 32;
int total_bar_length = 240;
int run_rectangle_length = 0;
void InfoBar(int offset);
void InfoBarRtc(int offset);
void sdCardInfo(void);
const char* gpsChip(int longname);
char bar_info[8] = "info";

void Speed_font0(
    const String& message1,
    const String& message2,
    float speed1,
    float speed2,
    float speed,
    int screen
) {
    // -------------------------------------------------
    // Formatting rules
    // -------------------------------------------------
    const int decimals_small = (screen == 2) ? 0 : 1;
    const int decimals_big   = 1;

    // -------------------------------------------------
    // Top row: labels + small speeds
    // -------------------------------------------------
    display.setFont(Fonts::Body12);
    display.setTextColor(GxEPD_BLACK);

    // Left label
    display.setCursor(offset, Layout::ROW12(1));
    display.print(message1);

    // Left value
    if (screen <= 2) {
        display.setFont(Fonts::Body18);
        display.print(speed1, decimals_small);
    }

    // Right label
    display.setFont(Fonts::Body12);
    display.setCursor(offset + 122, Layout::ROW12(1));
    display.print(message2);

    // Right value
    display.setFont(Fonts::Body18);
    display.print(speed2, decimals_small);

    // -------------------------------------------------
    // Big speed (main value)
    // -------------------------------------------------
    display.setFont(Fonts::SpeedXL);
    display.setCursor(offset, 120);
    display.print(speed, decimals_big);
}



void Speed_font1(String message1, String message2, float speed1, float speed2, float speed, int screen) {
  display.setCursor(offset, 36);
  if (screen == 0) {                         //Run "A" AVG
    display.setFont(Fonts::SpeedM);  //Test for bigger alfa fonts
    display.print(speed1, 1);                //last 10s max from run
    display.setFont(Fonts::Body12);
    display.setCursor(offset + 113, 36);
    display.print(message2);
    display.setFont(Fonts::SpeedM);
    display.print(speed2, 1);
  } else if (screen == 1) {  //Alfa screen, Gate xx Ex xx
    display.setFont(Fonts::Body12);
    display.print(message1);
    display.setFont(Fonts::SpeedM);
    display.print(speed1, 0);
    //display.setCursor(offset + 110, 36);
    display.setFont(Fonts::Body12);
    display.print(message2);
    display.setFont(Fonts::SpeedM);
    display.print(speed2, 0);
  } else if (screen == 2) {  //Alfa= xx.xx
    display.setFont(Fonts::Body18);
    display.print(message1);
    display.setFont(Fonts::SpeedM);
    display.print(speed1, 2);
  } else if (screen == 3) {  //Alfa = MISS
    display.setFont(Fonts::Body18);
    display.print(message1);
  }
  display.setFont(Fonts::SpeedXL);
  display.setCursor(offset, 120);
  display.println(speed, 1);
}
void Speed_font3(String message1, float speed) {
  display.setFont(&FreeSansBold24pt7b);
  display.setCursor(offset, 36);
  bar_position = 40;
  display.print(message1);
  display.setCursor(offset, 120);
  display.setFont(Fonts::SpeedXL);
  display.print(speed, 1);
}
int device_boot_log(int rows, int ws) {
  int r = 2, row = Layout::H9 + Layout::SPACING;
  display.setCursor(offset, Layout::ROW9(2));
  if (ws) delay(ws);
  display.print(E_paper_version);
  display.print(SW_version);
  if (rows == 2 || rows == 23 || rows == 24 || rows == 234) {
    display.setCursor(offset, Layout::ROW9(3));
    if (ws) delay(ws);
    sdCardInfo();
  }
  if (rows == 3 || rows == 23 || rows == 34 || rows == 234) {
    r = (rows == 23 || rows == 34) ? 3 : rows == 234 ? 4
                                                     : 2;
    if (ws) delay(ws);
    display.setCursor(offset, (rows == 234 || rows == 23 || rows == 34) ? Layout::ROW9(4) : Layout::ROW9(3));
    display.printf("Display size %dx%d\n", displayWidth, displayHeight);
  }
  if ((rows == 4 || rows == 24 || rows == 34 || rows == 234) && ubxMessage.monVER.hwVersion[0]) {
    r = (rows == 24 || rows == 34) ? 3 : rows == 234 ? 4
                                                     : 2;
    if (ws) delay(ws);
    display.setCursor(offset, rows == 24 || rows == 34 ? Layout::ROW9(4) : (rows == 234 ? Layout::ROW9(5) : Layout::ROW9(3)));
    display.printf("Gps %s at %dHz", gpsChip(1), config.sample_rate);
  }
  //display.updateWindow(0,Layout::ROW9(1)+1,175,r*row,true);
  return r;
}
#define DEVICE_BOOT_LOG(rows) device_boot_log(rows, 0)

void Boot_screen(void) {
  display.init();
  display.setRotation(1);
  displayHeight = display.height();
  displayWidth = display.width();
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  //display.drawExampleBitmap(ESP_GPS_LOGO_40;, offset + 198, 6, 40, 40, GxEPD_BLACK);
  InfoBarRtc(offset);
  display.setFont(Fonts::Body9);
  display.setCursor(offset, 14);
  if (RTC_voltage_bat < RTC_minimum_voltage_bat) {
    //int cursor = Layout::ROW9(2);
    display.println("EPS-GPS sleeping");
    display.print("Go back to sleep...");
    display.setFont(Fonts::Body12);
    display.setCursor(offset, 60);
    display.printf("Voltage to low: %.2f", RTC_voltage_bat);
    display.setCursor(offset, 80);
    display.println("Please charge lipo!");
    display.setCursor(offset, 100);
    display.print(RTC_Sleep_txt);
    display.update();
  } else {
    display.println("ESP-GPS booting");
    display.print(E_paper_version);
    display.println(SW_version);
    sdCardInfo();
    display.setCursor(offset, 102);
    display.printf("Logspace left : %d hour", Logtime_left(Free_space()) / 60);
    display.updateWindow(0, 0, displayWidth, displayWidth, true);
    delay(100);
    display.update();
  }
}
void Off_screen(int choice) {  //choice 0 = old screen, otherwise Simon screens
  //int offset=0;
  float session_time = (millis() - start_logging_millis) / 1000;
  display.setRotation(1);
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  int cursor = Layout::ROW9(3) + Layout::STEP12;
  ESP_GPS_LOGO_40;
  TOP_LEFT_TITLE_MSG("ESP-GPS saving");  //row1 14
  DEVICE_BOOT_LOG(4);
  display.setFont(Fonts::Body12);
  display.setCursor(offset, cursor);
  if (choice == 0) {
    if (Shut_down_Save_session == true) {
      display.println("Saving session");
      display.setFont(Fonts::Body9);
      display.setCursor(offset, (cursor += Layout::STEP9));
      display.print("Time: ");
      display.print(session_time, 0);
      display.print(" s");
      display.setCursor(offset, (cursor += Layout::STEP9));
      display.print("AVG: ");
      display.print(RTC_avg_10s, 2);
      display.setCursor(offset + 120, cursor);
      display.print("Dist: ");
      display.print(Ublox.total_distance / 1000, 0);
    } else {
      display.println("Going back to sleep");
    }
  }
  if (choice == 1) {
    if (Shut_down_Save_session == true) {
      display.println("Saving session");
      display.setFont(Fonts::Body9);
      display.setCursor(offset, (cursor += Layout::STEP9));
      display.print("Time: ");
      display.print(session_time, 0);
      display.print(" s");
      display.setCursor(offset, (cursor += Layout::STEP9));
      display.print("AVG: ");
      display.print(RTC_avg_10s, 2);
      display.setCursor(offset + 120, cursor);
      display.print("Dist: ");
      display.print(Ublox.total_distance / 1000, 0);
    } else {
      display.println("Going back to sleep");
    }
  }
  if (choice == 2) {  //shutdown due low bat
    display.println("Shutdown LOW Bat");
    display.setFont(Fonts::Body9);
    display.print("Bat = ");
    display.print(RTC_voltage_bat);
    display.println(" V");
  }
  InfoBar(offset);
  //display.update();
  display.updateWindow(0, 0, displayWidth, displayHeight, true);
  //delay(3000);  //om te voorkomen dat update opnieuw start !!!
}
//Screen in deepsleep, update bat voltage, refresh every 4000s !!
void Sleep_screen(int choice) {
  if (offset > 9) offset--;
  if (offset < 1) offset++;
  display.init();
  display.setRotation(1);
  displayHeight = display.height();
  displayWidth = display.width();
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  InfoBarRtc(offset);
  if (choice == 0) {
    display.setFont(Fonts::Body18);
    display.setCursor(offset, 24);
    display.print("Dist: ");
    display.println(RTC_distance, 0);
    display.setCursor(offset, 56);
    display.print("AVG: ");
    display.println(RTC_avg_10s, 2);
    display.setCursor(offset, 88);
    display.print("2s: ");
    display.print(RTC_max_2s);
    display.update();
  } else {
    int row1 = 15;
    int row = 15;
    int row2 = row1 + row;
    int row3 = row2 + row;
    int row4 = row3 + row;
    int row5 = row4 + row;
    int row6 = row5 + row;
    int col1 = 0 + offset;
    int col2 = 34 + offset;
    int col3 = 90 + offset;
    int col4 = 146 + offset;

    // Board Logo's:
    // add special logos - funlogos
    //Board logos
    if (RTC_Board_Logo == 1)  //Logo's Simon Dijkstra
      //display.drawExampleBitmap(Starboard_logoS_zwart, 195, 0, 48, 48, GxEPD_BLACK);
    if (RTC_Board_Logo == 2)
      //display.drawExampleBitmap(Fanatic_logoS_zwart, 195, 0, 48, 48, GxEPD_BLACK);
    if (RTC_Board_Logo == 3)
      //display.drawExampleBitmap(JP_logoS_zwart, 195, 0, 48, 48, GxEPD_BLACK);
    if (RTC_Board_Logo == 4)
      //display.drawExampleBitmap(NoveNove_logoS_zwart, 195, 0, 48, 48, GxEPD_BLACK);
    if (RTC_Board_Logo == 5)  //Logo's Jan Scholten
      //display.drawExampleBitmap(Mistral_logoS_zwart, 195, 0, 48, 48, GxEPD_BLACK);
    if (RTC_Board_Logo == 6)
      //display.drawExampleBitmap(Goya_logoS_zwart, 195, 0, 48, 48, GxEPD_BLACK);
    if (RTC_Board_Logo == 7)
      //display.drawExampleBitmap(Patrik_logoS_zwart, 195, 0, 48, 48, GxEPD_BLACK);
    if (RTC_Board_Logo == 8)
      //display.drawExampleBitmap(Severne_logoS_zwart, 195, 0, 48, 48, GxEPD_BLACK);
    if (RTC_Board_Logo == 9)
      //display.drawExampleBitmap(Tabou_logoS_zwart, 195, 0, 48, 48, GxEPD_BLACK);
    if (RTC_Board_Logo == 10)
      //display.drawExampleBitmap(F2_logo_zwart, 195, 0, 48, 48, GxEPD_BLACK);
    if (RTC_Board_Logo == 11)  // Schwechater - Austrian Beer - by tritondm
      //display.drawExampleBitmap(epd_bitmap_Schwechater, 195, -10, 79, 132, GxEPD_BLACK);
    if (RTC_Board_Logo == 12)
      //display.drawExampleBitmap(Thommen1_logo_BW, 195, 0, 64, 48, GxEPD_BLACK);
    if (RTC_Board_Logo == 13)
      //display.drawExampleBitmap(BIC_logo_BW, 195, 0, 48, 48, GxEPD_BLACK);
    if (RTC_Board_Logo == 14)
      //display.drawExampleBitmap(Carbon_art, 195, 0, 48, 39, GxEPD_BLACK);
    if (RTC_Board_Logo == 15)
      //display.drawExampleBitmap(FutureFly_logo_zwart, 195, 0, 48, 48, GxEPD_BLACK);
    if (RTC_Board_Logo == 16)
      //display.drawExampleBitmap(OneHundred_logoS_zwart, 195, 0, 48, 48, GxEPD_BLACK);
    if (RTC_Board_Logo == 17)
      //display.drawExampleBitmap(FMX_logoS_zwart, 195, 0, 48, 48, GxEPD_BLACK);
    if (RTC_Board_Logo == 18)
      //display.drawExampleBitmap(Phantom_logoS_zwart, 195, 0, 48, 48, GxEPD_BLACK);
    if (RTC_Board_Logo == 19)
      //display.drawExampleBitmap(f4_foils_logoS_zwart, 195, 0, 48, 48, GxEPD_BLACK);
    if (RTC_Board_Logo == 20)
      //display.drawExampleBitmap(Logo_LISA, 195, 0, 48, 48, GxEPD_BLACK);
    // Zeil Logo's:
    if (RTC_Sail_Logo == 1)  //Logo's Simon Dijkstra
      //display.drawExampleBitmap(GAsails_logoS_zwart, 195, 50, 48, 48, GxEPD_BLACK);
    if (RTC_Sail_Logo == 2)
      //display.drawExampleBitmap(DuoTone_logoS_zwart, 195, 50, 48, 48, GxEPD_BLACK);
    if (RTC_Sail_Logo == 3)
      //display.drawExampleBitmap(NP_logoS_zwart, 195, 50, 48, 48, GxEPD_BLACK);
    if (RTC_Sail_Logo == 4)
      //display.drawExampleBitmap(Pryde_logoS_zwart, 195, 50, 48, 48, GxEPD_BLACK);
    if (RTC_Sail_Logo == 5)  //Logo's Jan Scholten
      //display.drawExampleBitmap(Loftsails_logoS_zwart, 195, 50, 48, 48, GxEPD_BLACK);
    if (RTC_Sail_Logo == 6)
      //display.drawExampleBitmap(Gunsails_logoS_zwart, 195, 50, 48, 48, GxEPD_BLACK);
    if (RTC_Sail_Logo == 7)
      //display.drawExampleBitmap(Point7_logoS_zwart, 195, 50, 48, 48, GxEPD_BLACK);
    if (RTC_Sail_Logo == 8)
      //display.drawExampleBitmap(Simmer_logoS_zwart, 195, 50, 48, 48, GxEPD_BLACK);
    if (RTC_Sail_Logo == 9)
      //display.drawExampleBitmap(Naish_logoS_zwart, 195, 50, 48, 48, GxEPD_BLACK);
    if (RTC_Sail_Logo == 10) {
      //display.drawExampleBitmap(Severne_logoS_zwart, 195, 50, 48, 48, GxEPD_BLACK);
    }
    if (RTC_Sail_Logo == 11) {
      //display.drawExampleBitmap(S2maui_logoS_zwart, 195, 50, 48, 48, GxEPD_BLACK);
    }
    if (RTC_Sail_Logo == 12) {
      //display.drawExampleBitmap(North_Sails_logoS_zwart, 195, 50, 48, 48, GxEPD_BLACK);
    }
    if (RTC_Sail_Logo == 13) {
      //display.drawExampleBitmap(Challenger_Sails_logoS_zwart, 195, 50, 48, 48, GxEPD_BLACK);
    }
    if (RTC_Sail_Logo == 14) {
      //display.drawExampleBitmap(Phantom_logoS_zwart, 195, 50, 48, 30, GxEPD_BLACK);  //Patrik_logoS_zwart
    }
    if (RTC_Sail_Logo == 15) {
      //display.drawExampleBitmap(Patrik_logoS_zwart, 195, 50, 48, 48, GxEPD_BLACK);  //Patrik_logoS_zwart
    }
    if (RTC_Sail_Logo == 16) {
      //display.drawExampleBitmap(Logo_LISA_vertical, 195, 50, 48, 48, GxEPD_BLACK);  //Patrik_logoS_zwart
    }
    display.setCursor(col1, 105);  // was 121
    display.setFont(&SF_Distant_Galaxy9pt7b);
    display.print(RTC_Sleep_txt);

    display.setRotation(0);
    display.setCursor(30, 249);  //was 30, 249
    display.setFont(Fonts::Small6);
    if ((int)(calibration_speed * 100000) == 194) display.print("speed in knots");  //1.94384449 m/s to knots !!!
    if ((int)(calibration_speed * 1000000) == 3600) display.print("speed in km/h");

    display.setRotation(1);
    // left column
    display.setFont(Fonts::Mono9);
    display.setCursor(col1, row1);
    display.print("AV:");
    display.setCursor(col1, row2);
    display.print("R1:");
    display.setCursor(col1, row3);
    display.print("R2:");
    display.setCursor(col1, row4);
    display.print("R3:");
    display.setCursor(col1, row5);
    display.print("R4:");
    display.setCursor(col1, row6);
    display.print("R5:");

    display.setFont(Fonts::Body9);
    display.setCursor(col2, row1);
    display.println(RTC_avg_10s, 2);
    display.setCursor(col2, row2);
    display.println(RTC_R1_10s, 2);
    display.setCursor(col2, row3);
    display.println(RTC_R2_10s, 2);
    display.setCursor(col2, row4);
    display.println(RTC_R3_10s, 2);
    display.setCursor(col2, row5);
    display.println(RTC_R4_10s, 2);
    display.setCursor(col2, row6);
    display.println(RTC_R5_10s, 2);

    // right column
    display.setFont(Fonts::Mono9);
    display.setCursor(col3, row1);
    display.print("2sec:");
    display.setCursor(col3, row2);
    display.print("Dist:");
    display.setCursor(col3, row3);
    display.print("Alph:");
    display.setCursor(col3, row4);
    display.print("1h:");  //
    display.setCursor(col3, row5);
    display.print("NM:");
    display.setCursor(col3, row6);
    display.print("500m:");

    display.setFont(Fonts::Body9);
    display.setCursor(col4, row1);
    display.println(RTC_max_2s, 2);
    display.setCursor(col4, row2);
    display.println(RTC_distance, 2);
    display.setCursor(col4, row3);
    display.println(RTC_alp, 2);
    display.setCursor(col4, row4);
    display.println(RTC_1h, 2);  //
    display.setCursor(col4, row5);
    display.println(RTC_mile, 2);
    display.setCursor(col4, row6);
    display.println(RTC_500m, 2);
    display.update();
  }
}
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
  int posX = displayWidth - batW - 6;  //was -10
  int posY = displayHeight - batL;
  int line = 2;
  int seg = 3;
  int segW = batW - 2 * line;
  int segL = (batL - 0.25 * batW - 2 * line - (seg - 1)) / seg;
  display.fillRect(offset + posX, posY, 0.5 * batW, 0.25 * batW, GxEPD_BLACK);                 //battery top
  display.fillRect(offset + posX - 0.25 * batW, posY + 0.25 * batW, batW, batL, GxEPD_BLACK);  //battery body
  if (bat_perc < 67) display.fillRect(offset + posX - 0.25 * batW + line, posY + 0.25 * batW + line, segW, segL, GxEPD_WHITE);
  if (bat_perc < 33) display.fillRect(offset + posX - 0.25 * batW + line, posY + 0.25 * batW + line + 1 * (segL + 1), segW, segL, GxEPD_WHITE);
  if (bat_perc < 1) display.fillRect(offset + posX - 0.25 * batW + line, posY + 0.25 * batW + line + 2 * (segL + 1), segW, segL, GxEPD_WHITE);
  //Serial.printf("info bar cursor pos: %d, display height: %d\n", INFO_BAR_ROW,displayHeight);
  display.setFont(Fonts::Body9);
  //display.setCursor(displayWidth-8,(INFO_BAR_ROW-ROW_9PT));
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
const char* gpsChip(int longname) {
  switch (config.ublox_type) {
    case M8_9600BD:
      return longname ? "M8 9.6Kbd" : "M8";
      break;
    case M8_38400BD:
      return longname ? "M8 38.4Kbd" : "M8";
      break;
    case M8_115200BD:
      return longname ? "M8 115.2Kbd" : "M8";
      break;
    case M9_9600BD:
      return longname ? "M9 9.6Kbd" : "M9";
      break;
    case M9_38400BD:
      return longname ? "M9 38.4Kbd" : "M9";
      break;
    case M9_115200BD:
      return longname ? "M9 115.2Kbd" : "M9";
      break;
    case M10_9600BD:
      return longname ? "M10 9.6Kbd" : "M10";
      break;
    case M10_38400BD:
      return longname ? "M10 38.4Kbd" : "M10";
      break;
    case M10_115200BD:
      return longname ? "M10 115.2Kbd" : "M10";
      break;
    default:
      return "unknown";
      break;
  }
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
void sdCardInfo(void) {
  if (sdOK) display.printf("SD : %d Mb\n", freeSpace);
  if (LITTLEFS_OK) display.printf("Local : %d kb\n", (LITTLEFS.totalBytes() - LITTLEFS.usedBytes()) / 1024);
  // display.setCursor(offset,102);
  // display.printf("Logspace left : %d hour",Logtime_left(Free_space())/60);
}
void Stats_4lines(String Message1, String Message2, String Message3, String Message4, float Value1, float Value2, float Value3, float Value4) {
#define STAT4_ROW2 150
#define DIS_WIDTH 240
  display.setCursor(offset, Layout::ROW18(1));
  display.setFont(Fonts::Body18);
  display.print(Message1);
  display.setCursor(STAT4_ROW2, Layout::ROW18(1));
  // display.setFont(Fonts::Body18);
  if (Value1 < 100.0) display.println(Value1, 2);
  else if (Value1 < 1000.0) display.println(Value1, 1);
  else display.println(Value1, 0);
  display.setCursor(offset, Layout::ROW18(2));
  // display.setFont(Fonts::Body12);
  display.print(Message2);
  display.setCursor(STAT4_ROW2, Layout::ROW18(2));
  // display.setFont(Fonts::Body18);
  display.println(Value2, 2);
  display.setCursor(offset, Layout::ROW18(3));
  //display.setFont(Fonts::Body12);
  display.print(Message3);
  display.setCursor(STAT4_ROW2, Layout::ROW18(3));
  // display.setFont(Fonts::Body18);
  display.println(Value3, 2);
  display.setCursor(offset, Layout::ROW18(4));
  //display.setFont(Fonts::Body12);
  display.print(Message4);
  display.setCursor(STAT4_ROW2, Layout::ROW18(4));
  // if(Value4<0) display.print("-");
  //display.setFont(Fonts::Body18);
  //if(Value4>=0) display.println(Value4,2);
  display.println(Value4, 2);
}
void Stats_2s_3_lines(String Message1, String Message2, String Message3, float Value1, float Value2, float Value3) {
  display.setFont(Fonts::Body12);
  display.setCursor(offset, Layout::ROW18(1));
  display.print("2l: ");
  display.setFont(Fonts::Body18);
  display.print(S2.display_last_run * calibration_speed, 1);  //last 2s max from rundisplay.print(S2.avg_speed[9]*calibration_speed);
  display.setFont(Fonts::Body12);
  display.setCursor(120 + offset % 2, Layout::ROW18(1));  //zodat SXX niet groter wordt dan 244 pix
  display.print("2s: ");
  display.setFont(Fonts::Body18);
  display.print(S2.display_speed[9] * calibration_speed);  //best 2s, was avg_speed[9]
  display.setCursor(offset, Layout::ROW18(2));
  display.print(Message1);
  display.println(Value1);  //best 10s(Fast), was avg_speed[9]
  display.setCursor(offset, Layout::ROW18(3));
  display.print(Message2);
  display.println(Value2);  //langzaamste 10s(Slow) run van de sessie
  display.setCursor(offset, Layout::ROW18(4));
  display.print(Message3);
  display.println(Value3);  //average 5*10s
}
















void Update_screen(int screen) {
  static int count, old_screen, update_delay;

  update_time();
  update_epaper = 1;

  offset += ((count / 4) % 20 < 10) ? 1 : -1;
  if (offset < 0 || offset > 10) offset = 0;

  int cursor = 0;

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    if (screen != SPEED && screen != STATS9 && screen != STATS8 &&
        screen != STATSA && screen != STATSD)
      InfoBar(offset);

    if (screen == BOOT_SCREEN) {
      update_delay = 1000;
      ESP_GPS_LOGO_40;
      TOP_LEFT_TITLE_MSG("ESP-GPS config");
      DEVICE_BOOT_LOG(234);
      if (screen != old_screen) count = 0;
      Speed_in_Unit(offset);
    }

    if (screen == GPS_INIT_SCREEN) {
      update_delay = 100;
      ESP_GPS_LOGO_40;
      TOP_LEFT_TITLE_MSG("ESP-GPS GPS init");
      DEVICE_BOOT_LOG(24);

      display.setFont(Fonts::Body12);
      if (config.ublox_type == 0xFF) {
        display.setCursor(offset, cursor = Layout::ROW9(4) + Layout::STEP12);
        display.print("Auto detect gps-type");
      } else if (!ubxMessage.monVER.hwVersion[0]) {
        display.setCursor(offset, cursor = Layout::ROW9(3) + Layout::STEP12);
        display.println("Gps initializing");
        if (config.M10_high_nav == M10_HIGH_NAV_RATE) display.println("M10 high nav mode !");
        if (config.M10_high_nav == M10_DEFAULT_NAV)  display.println("M10 default nav mode");
      }

      if (screen != old_screen) count = 0;
      Speed_in_Unit(offset);
    }

  } while (display.nextPage());

  if (screen == BOOT_SCREEN) delay(1000);



    if (screen == WIFI_ON) {
    update_delay = 100;
    offset += (count % 20 < 10) ? 1 : -1;

    ESP_GPS_LOGO_40;
    TOP_LEFT_TITLE_MSG("ESP-GPS connect");
    DEVICE_BOOT_LOG(2);

    if (!SoftAP_connection) {
      display.setCursor(offset, 102);
      display.printf("Logspace left : %d hour", Logtime_left(Free_space()) / 60);
    }

    if (Wifi_on == 1) {
      display.setFont(Fonts::Body12);
      display.setCursor(offset, cursor = Layout::ROW9(3) + Layout::STEP12);
      display.print("Ssid: ");
      display.print(SoftAP_connection ? "ESP32AP" : actual_ssid);

      display.setFont(Fonts::Body9);
      display.setCursor(offset, cursor += Layout::STEP9);
      if (SoftAP_connection) {
        display.print("Password: password");
        display.setCursor(offset, cursor += Layout::STEP9);
      }
      display.printf("http://%s", IP_adress.c_str());

    } else {
      display.fillRect(0, 0, 250, 122, GxEPD_WHITE);

      ESP_GPS_LOGO_40;
      TOP_LEFT_TITLE_MSG("ESP-GPS ready");
      DEVICE_BOOT_LOG(24);

      display.setFont(Fonts::Body12);
      display.setCursor(offset, cursor = Layout::ROW9(4) + Layout::STEP12);

      if (ubxMessage.navPvt.numSV < 5) {
        display.println("Waiting for Sat >=5");
        display.setFont(Fonts::Body9);
        display.setCursor(offset, 102);
        display.println("Please go outside...       ");
      } else {
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
      InfoBar(offset);
    }
  }


  if (screen == WIFI_STATION) {
    update_delay = 100;
    ESP_GPS_LOGO_40;
    TOP_LEFT_TITLE_MSG("ESP-GPS try to connect");
    DEVICE_BOOT_LOG(2);

    display.setCursor(offset, 102);
    display.printf("Logspace left : %d hour", Logtime_left(Free_space()) / 60);

    display.setFont(Fonts::Body12);
    display.setCursor(offset, cursor = Layout::ROW9(3) + Layout::STEP12);
    display.print(actual_ssid);

    display.setFont(Fonts::Body9);
    display.setCursor(offset, cursor += Layout::STEP9);
    display.printf("For AP: use magnet in %ds", wifi_search);

    if (screen != old_screen) count = 0;
  }

  if (screen == WIFI_SOFT_AP) {
    update_delay = 100;
    ESP_GPS_LOGO_40;
    TOP_LEFT_TITLE_MSG("Connect to ESP-GPS");
    DEVICE_BOOT_LOG(2);

    display.setFont(Fonts::Body12);
    display.setCursor(offset, cursor = Layout::ROW9(3) + Layout::STEP12);
    display.print("Ssid: ESP32AP");

    display.setFont(Fonts::Body9);
    display.setCursor(offset, cursor += Layout::STEP9);
    display.print("Password: password");

    display.setCursor(offset, cursor += Layout::STEP9);
    display.printf("http://%s/ in %ds\n", IP_adress.c_str(), wifi_search);

    if (screen != old_screen) count = 0;
  }

   if (screen == SPEED) {
    update_delay = 50;

    int field = config.field_actual;
    bool alfa_screen =
      (Ublox.alfa_distance / 1000 < 350) && (abs(alfa_window) < 100);
    bool nautical_mile_screen =
      (Ublox.alfa_distance / 1000 > 1852);
    bool x_10km_screen =
      ((int)(Ublox.total_distance / 1000000) % 10 == 0) &&
      (Ublox.alfa_distance / 1000 > 1000);

    display.setFont(Fonts::Small6);
    display.setCursor(displayWidth - 20, INFO_BAR_TOP);
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
        int komma = int(gps_speed * calibration_speed * 10) % 10;
        display.setFont(Fonts::Huge75);
        display.setCursor(offset - 6, 115);
        display.print(int(gps_speed * calibration_speed));
        display.setFont(Fonts::Big30);  display.print(".");
        display.setFont(Fonts::SpeedL); display.println(komma);
      }
    } else {
      display.setFont(Fonts::Body18);
      display.setCursor(offset, 60);
      display.print("Low GPS signal !");
    }

    if (field <= SPEED2) {
      float run = S10.display_last_run * calibration_speed;
      float avg = S10.avg_5runs * calibration_speed;
      float cur = gps_speed * calibration_speed;

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
      float cur = gps_speed * calibration_speed;
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

    float cur = gps_speed * calibration_speed;

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
      double speed = gps_speed * calibration_speed;
      display.setFont(Fonts::Huge75);
      display.setCursor(offset - 6, 118);
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
      float cur   = gps_speed * calibration_speed;

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
                  gps_speed * calibration_speed, 1);

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
      display.setCursor(offset + 186, 48);
      display.print(bar_info);
    }

    if (config.speed_large_font == 0) {
      display.setFont(Fonts::Body12);
      display.setCursor(offset + 180, 43);
      display.print(time_now);
    }

    if (config.speed_large_font == 2 && run_rectangle_length < 160) {
      display.setTextWrap(false);
      display.setFont(Fonts::Mono9);
      display.setCursor(offset + 186, 10);
      display.print(bar_info);
    }

    display.fillRect(offset, bar_position, run_rectangle_length, 8, GxEPD_BLACK);
  }

  if (screen == TROUBLE) {
    display.setFont(Fonts::Body12);
    display.setCursor(offset, Layout::ROW12(1));
    display.println("No GPS frames for");
    display.println("more then 10 s.... ");
  }

  if (screen >= STATS1) {
    display.setFont(Fonts::Small6);
    display.setCursor(displayWidth - 20, INFO_BAR_TOP);
    display.print((char)screen);
  }

  if (screen == STATS1)
    Stats_2s_3_lines("10sF: ", "10sS: ", "AVG:  ",
                     S10.display_speed[9] * calibration_speed,
                     S10.display_speed[5] * calibration_speed,
                     S10.avg_5runs * calibration_speed);

  if (screen == STATS2) {
    static bool toggle;
    Stats_4lines("Dist: ", "1852m: ", toggle ? "3600s: " : "1800s: ", "Alfa: ",
                 Ublox.total_distance / 1000,
                 M1852.display_speed[9] * calibration_speed,
                 (toggle ? S3600.display_max_speed : S1800.display_max_speed) * calibration_speed,
                 A500.avg_speed[9] * calibration_speed);
    toggle = !toggle;
  }

  if (screen == STATS3)
    Stats_4lines("100m:", "250m:", "500m:", "Alfa:",
                 M100.display_speed[9] * calibration_speed,
                 M250.display_speed[9] * calibration_speed,
                 M500.display_speed[9] * calibration_speed,
                 A500.avg_speed[9] * calibration_speed);

 






    if (screen == STATS4) {
    display.setFont(Fonts::Body12);
    display.setCursor(offset, Layout::ROW18(1));
    display.print("10s Avg: ");
    display.setFont(Fonts::Body18);
    display.println(s10.avg_5runs * calibration_speed, 2);

    for (int i = 9; i > 6; i--) {
      int y = Layout::ROW18(2) + (9 - i) * Layout::STEP18;

      display.setCursor(offset, y);
      display.setFont(Fonts::Body12);
      display.print("R"); display.print(10 - i); display.print(" ");
      display.setFont(Fonts::Body18);
      display.print(s10.display_speed[i] * calibration_speed, 1);

      display.setCursor(offset + 118, y);
      if (i > 7) {
        display.setFont(Fonts::Body12);
        display.print(" R"); display.print(13 - i); display.print(" ");
        display.setFont(Fonts::Body18);
        display.print(s10.display_speed[i - 3] * calibration_speed, 1);
      } else {
        display.print(time_now);
      }
    }
  }


    if (screen == STATS5) {
    display.setFont(Fonts::Body12);
    display.setCursor(offset, Layout::ROW18(1));
    display.print("Last Alfa stats ! ");

    for (int i = 9; i > 6; i--) {
      int y = Layout::ROW18(2) + (9 - i) * Layout::STEP18;

      display.setCursor(offset, y);
      display.setFont(Fonts::Body12);
      display.print("A"); display.print(10 - i); display.print(" ");
      display.setFont(Fonts::Body18);
      display.print(a500.avg_speed[i] * calibration_speed, 1);

      if (i > 7) {
        display.setCursor(offset + 118, y);
        display.setFont(Fonts::Body12);
        display.print(" A"); display.print(13 - i); display.print(" ");
        display.setFont(Fonts::Body18);
        display.print(a500.avg_speed[i - 3] * calibration_speed, 1);
      }
    }
  }


  if (screen == STATS6) {
    Serial.println("STATS6_Simon_screen");

    int row1 = 15, step = 17;
    int row[6] = { row1,
                   row1 + step,
                   row1 + 2 * step,
                   row1 + 3 * step,
                   row1 + 4 * step,
                   row1 + 5 * step };

    int col1 = offset;
    int col2 = offset + 46;
    int col3 = offset + 114;
    int col4 = offset + 182;

    float s10[6] = {
      S10.avg_5runs * calibration_speed,
      S10.display_speed[9] * calibration_speed,
      S10.display_speed[8] * calibration_speed,
      S10.display_speed[7] * calibration_speed,
      S10.display_speed[6] * calibration_speed,
      S10.display_speed[5] * calibration_speed
    };

    float right[6] = {
      S2.display_speed[9] * calibration_speed,
      S10.s_max_speed * calibration_speed,
      Ublox.total_distance / 1000000.0,
      A500.avg_speed[9] * calibration_speed,
      M500.display_speed[9] * calibration_speed,
      M1852.display_speed[9] * calibration_speed
    };

    const char *left_lbl[6]  = { "AV:", "R1:", "R2:", "R3:", "R4:", "R5:" };
    const char *right_lbl[6] = { "2sec:", "Prv :", "Dist:", "Alp :", "500m:", "NM:" };

    display.setFont(Fonts::Mono12);
    for (int i = 0; i < 6; i++) {
      display.setCursor(col1, row[i]); display.print(left_lbl[i]);
      display.setCursor(col3, row[i]); display.print(right_lbl[i]);
    }

    display.setFont(Fonts::Body12);
    for (int i = 0; i < 6; i++) {
      display.setCursor(col2, row[i]); display.println(s10[i], 2);
      display.setCursor(col4, row[i]); display.println(right[i], i == 2 ? 0 : 2);
    }

    float prv = S10.s_max_speed * calibration_speed;
    int line = row[5];
    for (int i = 1; i < 6; i++)
      if (prv > s10[i]) { line = row[i - 1]; break; }

    display.fillRect(0, line + 2, col3 - 10, 2, GxEPD_BLACK);
  }

    if (screen == STATS7) {
    Serial.println("STATS7_Simon_bar graph");

    const int posX = 5, posY = INFO_BAR_TOP, GraphWidth = 215;
    const int MaxBars = NR_OF_BAR;
    int barSpace = 2, barWidth = 3, barPitch;
    static int r;

    int top = S10.display_speed[9] * calibration_speed;
    int max_bar = max(int(top / 5 + 1) * 5, 24);
    int step = (max_bar > 45) ? 5 : 3;
    int min_bar = max_bar - step * 8;
    float scale = 80.0f / (max_bar - min_bar);

    display.setFont(Fonts::Body9);
    display.setCursor(0, 15);
    display.println("Graph : Speed runs (10sec)");

    r = run_count % MaxBars + 1;

    display.setFont(Fonts::Small6);
    for (int i = 0; i < 9; i++) {
      int y = posY - i * 10;
      display.fillRect(offset + posX, y, GraphWidth, 1, GxEPD_BLACK);
      display.setCursor(offset + 225, y);
      display.print(min_bar + i * step);
    }

    display.setCursor(0, 26);
    display.print("R1-R5:");
    for (int i = 9; i > 4; i--) {
      display.print(S10.display_speed[i] * calibration_speed);
      display.print(i > 5 ? " " : "");
    }
    display.println();

    int bars = (run_count < MaxBars) ? r : MaxBars;
    barWidth = max((GraphWidth - bars * barSpace) / bars, 3);
    barPitch = barWidth + barSpace;

    for (int i = 0; i < bars; i++) {
      int idx = (run_count < MaxBars) ? i : (i + r) % MaxBars;
      int h = (S10.speed_run[idx] * calibration_speed - min_bar) * scale;
      display.fillRect(offset + posX + i * barPitch, posY - h, barWidth, h, GxEPD_BLACK);
    }
  }


     if (screen == STATS8 || screen == STATS9 || screen == STATSA) {
      display.setFont(Fonts::Body12);

      for (int i = 9; i > 4; i--) {
        int y = 24 * (10 - i);
        display.setCursor(offset, y);

        if (screen == STATS8) {
          display.print("500 "); display.print(10 - i); display.print(": ");
          display.print(M500.avg_speed[i] * calibration_speed, 2);
          display.print(" @");
          display.print(M500.time_hour[i]);
          display.print(M500.time_min[i] < 10 ? ":0" : ":");
          display.print(M500.time_min[i]);
        }

        else if (screen == STATS9) {
          display.print("Run "); display.print(10 - i); display.print(": ");
          display.print(S10.avg_speed[i] * calibration_speed, 2);
          display.print(" @");
          display.print(S10.time_hour[i]);
          display.print(S10.time_min[i] < 10 ? ":0" : ":");
          display.print(S10.time_min[i]);
        }

        else { // STATSA
          display.print("2s: "); display.print(10 - i); display.print(": ");
          display.print(S2.avg_speed[i] * calibration_speed, 2);
          display.print(" @");
          display.print(S2.time_hour[i]);
          display.print(S2.time_min[i] < 10 ? ":0" : ":");
          display.print(S2.time_min[i]);
        }
      }
    }

    if (screen == STATSB)
      Stats_2s_3_lines("10sLast: ", "10sBest: ", "AVG :  ",
                       S10.display_last_run * calibration_speed,
                       S10.display_speed[9] * calibration_speed,
                       S10.avg_5runs * calibration_speed);

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