#pragma once

#include <Arduino.h>
#include <time.h>
#include "Definitions.h"
#include "GPS/GPS_data.h"


// ============================================================================
// SYSTEM / NETWORK STATE
// ============================================================================
extern bool downloading_file;
extern int  wifi_search;

// ============================================================================
// GPS / NAVIGATION STATE
// ============================================================================
extern bool GPS_Signal_OK;
extern char Ublox_type[20];
extern int  last_gps_msg;
extern int  nav_pvt_message;
extern int  nav_sat_message;
extern int  old_message;
extern int  msgType;

// ============================================================================
// TIME / CLOCK / SYNC
// ============================================================================
extern int  NTP_time_set;
extern int  Gps_time_set;
extern char TimeZone[64];
extern tm   tmstruct;
extern int  Time_Set_OK;
extern long _timezone;

// ============================================================================
// DEVICE / FIRMWARE IDENTITY
// ============================================================================
extern byte mac[6];
extern const char SW_version[16];

// ============================================================================
// GPS RUN / STATISTICS
// ============================================================================
extern int   first_fix_GPS;
extern int   run_count;
extern int   old_run_count;
extern int   stat_count;
extern int   S10_previous_run;

extern int   gps_speed_value;
extern float alfa_window;
extern float Mean_heading;
extern float heading_SD;
extern float calibration_speed;

// ============================================================================
// LOGGING / TIMING
// ============================================================================
extern int  start_logging_millis;
extern int  next_gpy_full_frame;
extern bool GPS_logging;

// ============================================================================
// UI / SCREEN STATE
// ============================================================================
extern int GPIO12_screen;

// ============================================================================
// BATTERY / POWER MONITORING
// ============================================================================
extern float analog_mean;

// ============================================================================
// WATCHDOG / DIAGNOSTICS
// ============================================================================
extern int wdt_task0;
extern int wdt_task1;
extern int max_count_wdt_task0;

// ============================================================================
// SHUTDOWN / SESSION CONTROL
// ============================================================================
extern bool Shut_down_Save_session;
extern bool reset_boot;

// ============================================================================
// GPS OBJECTS (defined in gps_manager.cpp)
// ============================================================================
extern GPS_speed     M100;
extern GPS_speed     M250;
extern GPS_speed     M1852;

extern GPS_time      S2;
extern GPS_time      s2;
extern GPS_time      S10;
extern GPS_time      s10;

extern Alfa_speed    A250;

extern GPS_data      Ublox;
extern GPS_SAT_info  Ublox_Sat;
