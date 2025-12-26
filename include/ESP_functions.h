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

//#include "GxEPD.h"

// Forward declarations
class GPS_data;
class GPS_SAT_info;
class GPS_speed;
class GPS_time;
class Alfa_speed;
class GPS_Track;
class FtpServer;


extern const char* filename;
extern const char* filename_backup;


// Power control
void Shut_down();

// FreeRTOS tasks
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern TaskHandle_t t1;




extern String IP_adress;
extern const char SW_version[16];

extern char Ublox_type[20];
extern char TimeZone[64];

extern int sdTrouble;
extern bool button;
extern bool LITTLEFS_OK;
extern bool reed;
extern bool deep_sleep;
extern bool Wifi_on;
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

extern GPS_speed M100;
extern GPS_speed M250;
extern GPS_speed M500;
extern GPS_speed M1852;

extern GPS_time S2;
extern GPS_time s2;
extern GPS_time S10;
extern GPS_time s10;
extern GPS_time S1800;
extern GPS_time S3600;

extern Alfa_speed A250;
extern Alfa_speed A500;
extern Alfa_speed a500;

extern GPS_Track M_500;

// -------------------------------------------------
// Functions
// -------------------------------------------------
void go_to_sleep(uint64_t sleep_time, bool refresh_screen);
void Update_bat(void);
void taskOne(void *parameter);
void taskTwo(void *parameter);
void GPSTC_info(char* GPSTC_post);
void print_wakeup_reason();
void print_reset_reason(int reason);
void Search_for_wifi(void);
void printLocalTime();
void feedTheDog_Task0();
void feedTheDog_Task1();
void OnWiFiEvent(WiFiEvent_t event);




