// ============================================================================
// E_paper.cpp
// - Owns physical e-paper object
// - Draw chrome (battery/sats/time)
// - Boot/diagnostics helpers
// IMPORTANT: Display code must be READ-ONLY for time
// ============================================================================

#include <Arduino.h>
#include "Display/E_paper.h"
#include "Fonts.h"

#include "Ublox/ublox.h"
#include "GPS/GPS_data.h"
#include "Definitions.h"
#include "Globals.h"

#include <LittleFS.h>
#include "Storage/storage_manager.h"
#include "system_info.h"
#include "Layout.h"
#include "Display/screen_system.h"
#include "MANAGERS/config_manager.h"
#include "task_display.h"

// ============================================================================
// Display instance (OWNED HERE)
// ============================================================================
GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(
  GxEPD2_213_B74(ELINK_SS, ELINK_DC, ELINK_RESET, ELINK_BUSY)
);

int16_t displayWidth=0, displayHeight=0;

// ============================================================================
// Local UI state
// ============================================================================
static int ui_offset=0;

#define INFO_BAR_HEIGHT 15
#define INFO_BAR_ROW (display.height()-2)

// ============================================================================
// Chrome helpers
// ============================================================================
void Bat_level_Simon(int ui_offset);
void Sats_level(int ui_offset);
int  Time(int ui_offset);
int  DateTimeRtc(int ui_offset);

void drawChrome(int offset,bool rtcMode){
  Bat_level_Simon(offset);
  if(rtcMode){ DateTimeRtc(offset); }
  else{ Sats_level(offset); Time(offset); }
}

// ============================================================================
// Boot / diagnostics drawing
// ============================================================================
int device_boot_log(int rows,int ws){
  int r=2;
  auto pause=[&](){ if(ws) delay(ws); };

  display.setCursor(ui_offset,Layout::ROW9(2)); pause(); display.print(SW_version);

  const bool show_storage=(rows==2||rows==23||rows==24||rows==234);
  if(show_storage){
    display.setCursor(ui_offset,Layout::ROW9(3)); pause(); sdCardInfo();
  }

  const bool advance_row=(rows==3||rows==23||rows==34||rows==234);
  if(advance_row){
    r=(rows==234)?4:((rows==23||rows==34)?3:2);
    display.setCursor(ui_offset,
      (rows==234||rows==23||rows==34)?Layout::ROW9(4):Layout::ROW9(3));
  }

  const bool show_gps=(rows==4||rows==24||rows==34||rows==234);
  if(show_gps){
    display.setCursor(ui_offset,
      (rows==234)?Layout::ROW9(5):((rows==24||rows==34)?Layout::ROW9(4):Layout::ROW9(3)));
    display.printf("Gps %s at %dHz",gpsChip(1),systemInfo.sample_rate);
  }

  return r;
}
#define device_boot_log(rows) device_boot_log(rows,0)

// ============================================================================
// Time helpers (READ-ONLY)
// ============================================================================
char time_now[8];
char time_now_sec[12];

// NOTE: Do NOT call Set_GPS_Time() here.
// Time is latched in task_gps (Task1) exactly once.
int update_time(){
  if(!getLocalTime(&tmstruct)) return 1;
  sprintf(time_now,"%02d:%02d",tmstruct.tm_hour,tmstruct.tm_min);
  sprintf(time_now_sec,"%02d:%02d:%02d",tmstruct.tm_hour,tmstruct.tm_min,tmstruct.tm_sec);
  return 0;
}

// ============================================================================
// Battery / satellite / time chrome
// ============================================================================
void Bat_level_Simon(int ui_offset){
  float bat_perc=100.0f*(1.0f-(VOLTAGE_100-RTC_voltage_bat)/(VOLTAGE_100-VOLTAGE_0));
  bat_perc=constrain(bat_perc,0,100);

  int batW=8, batL=15;
  int posX=display.width()-batW-6;
  int posY=display.height()-batL;

  display.fillRect(ui_offset+posX,posY,batW/2,batW/4,GxEPD_BLACK);
  display.fillRect(ui_offset+posX-batW/4,posY+batW/4,batW,batL,GxEPD_BLACK);

  display.setFont(Fonts::Body9);
  display.setCursor(ui_offset+146,INFO_BAR_ROW);
  display.print(RTC_voltage_bat+0.04,1);
  display.print("V ");
  display.print((int)bat_perc);
  display.print("%");
}

void Sats_level(int ui_offset){
  int satnum=ubxMessage.navPvt.numSV;
  display.setFont(Fonts::Body9);
  display.setCursor(120+ui_offset-(satnum<10?9:18),INFO_BAR_ROW);
  display.print(satnum);
}

int Time(int ui_offset){
  display.setFont(Fonts::Body9);
  display.setCursor(ui_offset,INFO_BAR_ROW);

  // If system time not set yet, show placeholder (or you can show RTC)
  if(update_time()){ display.print("--:--"); return 1; }

  display.print(time_now);
  return 0;
}

int DateTimeRtc(int ui_offset){
  display.setFont(Fonts::Body9);
  display.setCursor(ui_offset,INFO_BAR_ROW);
  display.printf("%02d:%02d %02d-%02d-%02d",RTC_hour,RTC_min,RTC_day,RTC_month,RTC_year);
  return 0;
}
