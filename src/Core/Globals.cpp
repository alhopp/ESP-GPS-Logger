#include <Arduino.h>
#include "Globals.h"
#include "Definitions.h"

// Put your actual declarations exactly as in original

bool wifi_configured = false;
bool downloading_file = false;



Button_push Short_push12 (12, 50,   15, 1, 1);
Button_push Long_push12  (12, 2000, 10, 4, 1);

Button_push Short_push19 (GO_TO_SLEEP_PULLDOWN, 10,   10, 9, 0);
Button_push Long_push19  (GO_TO_SLEEP_PULLDOWN, 1700, 10, 9, 0);

Button_push Short_push39 (GO_TO_SLEEP_GPIO, 10,   10, 9, 1);
Button_push Long_push39  (GO_TO_SLEEP_GPIO, 1700, 10, 9, 1);

bool GPS_Signal_OK = false;
bool Field_choice  = false;

byte mac[6] = {0};   // ESP32 MAC address

const char SW_version[16]="Ver 6.01c";

char Ublox_type[20]="Ublox unknown...";


char TimeZone[64] ="GMT0";

bool Shut_down_Save_session   = false;

int  NTP_time_set             = 0;
int  Gps_time_set             = 0;

int last_gps_msg     = 0;
int nav_pvt_message  = 0;
int old_message      = 0;
int msgType          = 0;
int nav_sat_message = 0;



// -----------------------------------------------------------------------------
// GPS run / statistics
// -----------------------------------------------------------------------------
int first_fix_GPS;
int run_count;
int old_run_count;
int stat_count;
int S10_previous_run;

int gps_speed;
float alfa_window;
float Mean_heading;
float heading_SD;

// -----------------------------------------------------------------------------
// Timing / logging
// -----------------------------------------------------------------------------
int start_logging_millis;
int next_gpy_full_frame = 0;

// -----------------------------------------------------------------------------
// UI / screen / input
// -----------------------------------------------------------------------------
int GPIO12_screen = 0;   // keuze welk scherm

// -----------------------------------------------------------------------------
// Battery / power monitoring
// -----------------------------------------------------------------------------
int   analog_bat;
float analog_mean = 2000;
int   low_bat_count;

// -----------------------------------------------------------------------------
// Watchdog / diagnostics
// -----------------------------------------------------------------------------
int wdt_task0;
int wdt_task1;
int max_count_wdt_task0;

// -----------------------------------------------------------------------------
// Storage
// -----------------------------------------------------------------------------
int sdTrouble = 0;
int freeSpace;










