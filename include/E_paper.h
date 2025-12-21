#pragma once

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_213_B74.h>

extern GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display;

extern char time_now[8];
extern char time_now_sec[12];
extern int run_rectangle_length;
extern int bar_position;
extern int total_bar_length;
extern int bar_length;

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
//  Legacy logo macros (no-op, statement-safe)
//  These remain ONLY for backward compatibility
// ============================================================================

#ifndef ESP_GPS_LOGO_40
#define ESP_GPS_LOGO_40 do {} while (0)
#endif

#ifndef ESP_GPS_LOGO_48
#define ESP_GPS_LOGO_48 do {} while (0)
#endif

// ============================================================================
//  E-paper hardware pins (public constants)
// ============================================================================

#define ELINK_SS     5
#define ELINK_BUSY  4
#define ELINK_RESET 16
#define ELINK_DC    17

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

// Legacy screen aliases (ASCII-compatible IDs)
#define STATS1  49
#define STATS2  50
#define STATS3  51
#define STATS4  52
#define STATS5  53
#define STATS6  54
#define STATS7  55
#define STATS8  56
#define STATS9  57
#define STATSA  65
#define STATSB  66
#define STATSC  67
#define STATSD  68

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
#define SPEEDB  66
#define SPEEDC  67
#define SPEEDD  68

// ============================================================================
//  Display object (defined in E_paper.cpp ONLY)
// ============================================================================



// ============================================================================
//  Global runtime state (owned elsewhere)
// ============================================================================

extern int   sdTrouble;
extern int   gps_speed;
extern int   S10_previous_run;
extern int   wifi_search;
extern int   start_logging_millis;
extern int   freeSpace;

extern bool  sdOK;
extern bool  LITTLEFS_OK;
extern bool  Wifi_on;
extern bool  SoftAP_connection;
extern bool  GPS_Signal_OK;
extern bool  Shut_down_Save_session;

extern int   ftpStatus;
extern int   bootCount;
extern int   run_count;
extern int   stat_count;
extern int   GPIO12_screen;

extern float RTC_voltage_bat;
extern float RTC_minimum_voltage_bat;
extern float alfa_window;
extern double delta_heading;
extern double ref_heading;

extern String IP_adress;
extern String actual_ssid;

extern const char E_paper_version[];
extern const char SW_version[16];

extern UBXMessage ubxMessage;

// ============================================================================
//  RTC persistent values
// ============================================================================

extern RTC_DATA_ATTR int   offset;
extern RTC_DATA_ATTR float RTC_distance;
extern RTC_DATA_ATTR float RTC_avg_10s;
extern RTC_DATA_ATTR float RTC_max_2s;

// RTC stats
extern RTC_DATA_ATTR short RTC_year;
extern RTC_DATA_ATTR short RTC_month;
extern RTC_DATA_ATTR short RTC_day;
extern RTC_DATA_ATTR short RTC_hour;
extern RTC_DATA_ATTR short RTC_min;

extern RTC_DATA_ATTR float RTC_alp;
extern RTC_DATA_ATTR float RTC_500m;
extern RTC_DATA_ATTR float RTC_1h;
extern RTC_DATA_ATTR float RTC_mile;

extern RTC_DATA_ATTR float RTC_R1_10s;
extern RTC_DATA_ATTR float RTC_R2_10s;
extern RTC_DATA_ATTR float RTC_R3_10s;
extern RTC_DATA_ATTR float RTC_R4_10s;
extern RTC_DATA_ATTR float RTC_R5_10s;

extern RTC_DATA_ATTR int   RTC_counter;

// ============================================================================
//  Navigation / statistics objects
// ============================================================================

extern GPS_speed M100;
extern GPS_speed M250;
extern GPS_speed M500;

extern Alfa_speed A250;
extern Alfa_speed A500;
extern Alfa_speed a500;

extern GPS_time  S1800;
extern GPS_time  S3600;

extern GPS_Track M_500;

// ============================================================================
//  Public E-paper API
// ============================================================================

void Boot_screen(void);
void Off_screen(int choice);
void Sleep_screen(int choice);
void Update_screen(int screen);

void Bat_level(int X_offset, int Y_offset);
void Bat_level_Simon(int offset);
void Sat_level(int offset);
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


