// -----------------------------------------------------------------------------
// Globals.h
//
// Centralised global runtime state (declarations only).
//
// Notes:
// - This header declares ALL globals defined in Globals.cpp
// - No logic, no initialisation, no side-effects
// - Grouping and naming must match Globals.cpp exactly
// -----------------------------------------------------------------------------

#pragma once

#include <Arduino.h>
#include "Definitions.h"

// ============================================================================
// SYSTEM / NETWORK STATE
// ============================================================================
extern bool wifi_configured;
extern bool downloading_file;

// ============================================================================
// GPS / NAVIGATION STATE
// ============================================================================
extern bool GPS_Signal_OK;
extern bool Field_choice;

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

extern int   gps_speed;
extern float alfa_window;
extern float Mean_heading;
extern float heading_SD;

// ============================================================================
// LOGGING / TIMING
// ============================================================================
extern int start_logging_millis;
extern int next_gpy_full_frame;

// ============================================================================
// UI / SCREEN STATE
// ============================================================================
extern int GPIO12_screen;

// ============================================================================
// BATTERY / POWER MONITORING
// ============================================================================
extern int   analog_bat;
extern float analog_mean;
extern int   low_bat_count;

// ============================================================================
// STORAGE / FILESYSTEM
// ============================================================================
extern int sdTrouble;
extern int freeSpace;

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

