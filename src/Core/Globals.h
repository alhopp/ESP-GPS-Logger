#pragma once

#include <Arduino.h>
#include <time.h>
#include "Core/log.h"
#include "GPS/Data/gps_data.h"
#include "GPS/Data/gps_satellite_quality.h"


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


// ============================================================================
// LOGGING / TIMING
// ============================================================================
extern int  start_logging_millis;
extern int  next_gpy_full_frame;
extern bool GPS_logging;



// ============================================================================
// BATTERY / POWER MONITORING
// ============================================================================
extern float analog_mean;

// ============================================================================
// WATCHDOG / DIAGNOSTICS
// ============================================================================
extern int wdt_task0;
extern int wdt_task1;


// ============================================================================
// SHUTDOWN / SESSION CONTROL
// ============================================================================
extern bool Shut_down_Save_session;
extern bool reset_boot;
extern GPS_data      Ublox;
extern GPS_SAT_info  Ublox_Sat;

extern volatile bool woke_from_sleep;

