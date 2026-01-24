// -----------------------------------------------------------------------------
// Globals.cpp
//
// Centralised global runtime state.
//
// Notes:
// - This file contains ONLY data declarations
// - No logic, no side-effects, no initialisation sequences
// - Values are grouped by functional responsibility
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include "Core/Globals.h"
#include "Core/Definitions.h"

// ============================================================================
// SYSTEM / NETWORK STATE
// ============================================================================
bool wifi_configured   = false;
bool downloading_file  = false;
bool reset_boot = false;

// ============================================================================
// GPS / NAVIGATION STATE
// ============================================================================
bool GPS_Signal_OK = false;
char Ublox_type[20] = "Ublox unknown...";
int  last_gps_msg    = 0;
int  nav_pvt_message = 0;
int  nav_sat_message = 0;
int  old_message     = 0;
int  msgType         = 0;

// ============================================================================
// TIME / CLOCK / SYNC
// ============================================================================
int  NTP_time_set = 0;
int  Gps_time_set = 0;
char TimeZone[64] = "GMT0";

// ============================================================================
// DEVICE / FIRMWARE IDENTITY
// ============================================================================
const char SW_version[16] = "Ver 6.01c";

// ============================================================================
// GPS RUN / STATISTICS
// ============================================================================
int   first_fix_GPS;
int   run_count;
int   old_run_count;
int   stat_count;
int   S10_previous_run;
int   gps_speed_value;
float alfa_window;
float Mean_heading;
float heading_SD;

// ============================================================================
// LOGGING / TIMING
// ============================================================================
int start_logging_millis;
int next_gpy_full_frame = 0;



// ============================================================================
// BATTERY / POWER MONITORING
// ============================================================================
float analog_mean   = 2000;

// ============================================================================
// WATCHDOG / DIAGNOSTICS
// ============================================================================
int wdt_task0;
int wdt_task1;
int max_count_wdt_task0;

// ============================================================================
// SHUTDOWN / SESSION CONTROL
// ============================================================================
bool Shut_down_Save_session = false;

volatile bool woke_from_sleep = false;
