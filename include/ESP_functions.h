#pragma once

#include <Arduino.h>
#include <esp_attr.h>
#include <SPI.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <FS.h>

/* ==== PROJECT HEADERS (ORDER MATTERS) ==== */
#include "Definitions.h"
#include "GPS_data.h"
#include "Ublox.h"
#include "SD_card.h"
#include "ESP32FtpServerJH.h"
#include "E_paper.h"

extern const char* filename;
extern const char* filename_backup;

extern const char SW_version[16];

extern char Ublox_type[20];
extern char TimeZone[64];

extern int sdTrouble;
extern bool button;
extern bool LITTLEFS_OK;
extern bool reed;


extern bool SoftAP_connection;
extern bool GPS_Signal_OK;
extern bool Field_choice;
extern bool reset_boot;
extern bool Shut_down_Save_session;
extern bool trouble_screen;

extern bool downloading_file;
extern bool ap_mode;

extern int GPS_OK;
extern int analog_bat;
extern int first_fix_GPS, run_count, old_run_count, stat_count, GPS_delay;
extern int start_logging_millis;
extern int wifi_search;
extern int ftpStatus;
extern int last_gps_msg;
extern int nav_pvt_message;
extern int old_message;
extern int nav_sat_message;
extern int next_gpy_full_frame;
extern int msgType;
extern int GPIO12_screen;
extern int low_bat_count;
extern int gps_speed;
extern int S10_previous_run;

extern float alfa_window;
extern float analog_mean;
extern float Mean_heading, heading_SD;

extern int wdt_task0, wdt_task1;
extern int max_count_wdt_task0;

extern String actual_ssid;

// Network
extern byte mac[6];
extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;

// -------------------------------------------------
// Objects
// -------------------------------------------------
extern FtpServer ftpSrv;
extern GPS_data Ublox;
extern GPS_SAT_info Ublox_Sat;



// -------------------------------------------------
// Functions
// -------------------------------------------------

void printLocalTime();
void feedTheDog_Task0();
void feedTheDog_Task1();




