// ============================================================================
// Globals.h
//
// Declarations of runtime globals shared across the system.
// NOTE:
//  - This file declares globals only (extern).
//  - All definitions must live in exactly ONE .cpp file.
// ============================================================================

#pragma once

#include <Arduino.h>
#include "Button_push.h"


// ============================================================================
// Button instances (runtime globals)
// ============================================================================

extern Button_push Short_push12;
extern Button_push Long_push12;

extern Button_push Short_push19;
extern Button_push Long_push19;

extern Button_push Short_push39;
extern Button_push Long_push39;


// ============================================================================
// System / state flags
// ============================================================================

extern bool GPS_Signal_OK;
extern bool Field_choice;

extern bool Shut_down_Save_session;


// ============================================================================
// Network / identity
// ============================================================================

extern byte mac[6];
extern const char SW_version[16];


// ============================================================================
// GPS / time / localisation
// ============================================================================

extern char Ublox_type[20];
extern char TimeZone[64];

extern int  Gps_time_set;
extern int  NTP_time_set;


// -----------------------------------------------------------------------------
// GPS message tracking
// -----------------------------------------------------------------------------

extern int last_gps_msg;
extern int nav_pvt_message;
extern int nav_sat_message;

extern int old_message;
extern int msgType;


// -----------------------------------------------------------------------------
// GPS run / statistics
// -----------------------------------------------------------------------------

extern int   first_fix_GPS;

extern int   run_count;
extern int   old_run_count;
extern int   stat_count;
extern int   S10_previous_run;

extern int   gps_speed;

extern float alfa_window;
extern float Mean_heading;
extern float heading_SD;


// ============================================================================
// Timing / logging
// ============================================================================

extern int start_logging_millis;
extern int next_gpy_full_frame;


// ============================================================================
// UI / screen / input
// ============================================================================

// GPIO12: screen selection / mode
extern int GPIO12_screen;


// ============================================================================
// Battery / power monitoring
// ============================================================================

extern int   analog_bat;
extern float analog_mean;
extern int   low_bat_count;


// ============================================================================
// Watchdog / diagnostics
// ============================================================================

extern int wdt_task0;
extern int wdt_task1;
extern int max_count_wdt_task0;


// ============================================================================
// Storage
// ============================================================================

extern int sdTrouble;
extern int freeSpace;
