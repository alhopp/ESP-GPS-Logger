#pragma once

#include <Arduino.h>

#include <GxEPD2_BW.h>
#include <epd/GxEPD2_213_B74.h>

#include "rtc_state.h"
#include "Ublox/Ublox.h"

#include "GPS/GPS_data.h"

extern GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display;

extern char time_now[8];
extern char time_now_sec[12];
extern int run_rectangle_length;
extern int bar_position;
extern int total_bar_length;
extern int bar_length;

// -----------------------------------------------------------------------------
// UI chrome helpers (battery, satellites, time)
// -----------------------------------------------------------------------------
void drawChrome(int offset, bool rtcMode);

void sdCardInfo(void);

int device_boot_log(int rows, int ws);


struct UBXMessage;

class GPS_speed;
class Alfa_speed;
class GPS_time;
class GPS_Track;

// ==========================
// E-paper pin mapping
// ==========================
#define ELINK_SS     5
#define ELINK_DC     17
#define ELINK_RESET  16
#define ELINK_BUSY  4


// ============================================================================
//  Screen IDs (public API)
// ============================================================================

#define BOOT_SCREEN        0

#define SPEED              10
#define WIFI_ON            11
#define WIFI_STATION       12
#define WIFI_SOFT_AP       13
#define TROUBLE            15
#define GPS_INIT_SCREEN    16


#define SPEED1  49
#define SPEED2  50
#define SPEED3  51
#define SPEED4  52
#define SPEED5  53
#define SPEED6  54
#define SPEED7  55
#define SPEED8  56
#define SPEED9  57
#define SPEEDA  65


// ============================================================================
//  Global runtime state (owned elsewhere)
// ============================================================================

extern int   gps_speed_value;
extern int   S10_previous_run;
extern int   wifi_search;
extern int   start_logging_millis;


extern bool  sdOK;
extern bool  SoftAP_connection;

extern int   ftpStatus;
extern int   bootCount;
extern int   run_count;
extern int   stat_count;

extern float alfa_window;
extern double delta_heading;
extern double ref_heading;

extern String IP_adress;
extern String actual_ssid;



// ============================================================================
//  Public E-paper API
// ============================================================================


void Off_screen(int choice);
void Sleep_screen(int choice);


void Bat_level(int X_offset, int Y_offset);
void Bat_level_Simon(int ui_offset);
void Sat_level(int ui_offset);
void time_print(int time);

void Stats_4lines(
  String Message1,
  String Message2,
  String Message3,
  String Message4,
  float  Value1,
  float  Value2,
  float  Value3,
  float  Value4
);


