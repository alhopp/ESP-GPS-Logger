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
#include "Button_push.h"

#include "GxEPD.h"

// Forward declarations
class GPS_data;
class GPS_SAT_info;
class GPS_speed;
class GPS_time;
class Alfa_speed;
class GPS_Track;
class FtpServer;




// Display (defined elsewhere)
//extern GxEPD_Class display;
// -------------------------------------------------
// Global state (extern ONLY)
// -------------------------------------------------

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
// RTC memory (extern ONLY)
// -------------------------------------------------
extern RTC_DATA_ATTR float calibration_speed;
extern RTC_DATA_ATTR int offset;

extern RTC_DATA_ATTR float RTC_distance;
extern RTC_DATA_ATTR float RTC_avg_10s;
extern RTC_DATA_ATTR float RTC_max_2s;
extern RTC_DATA_ATTR float RTC_1h;
extern RTC_DATA_ATTR float RTC_alp;
extern RTC_DATA_ATTR float RTC_mile;

extern RTC_DATA_ATTR float RTC_avg_10s_knots;
extern RTC_DATA_ATTR float RTC_max_2s_knots;
extern RTC_DATA_ATTR float RTC_alp_knots;
extern RTC_DATA_ATTR float RTC_1h_knots;
extern RTC_DATA_ATTR float RTC_mile_knots;

extern RTC_DATA_ATTR short RTC_year;
extern RTC_DATA_ATTR short RTC_month;
extern RTC_DATA_ATTR short RTC_day;
extern RTC_DATA_ATTR short RTC_hour;
extern RTC_DATA_ATTR short RTC_min;
extern RTC_DATA_ATTR float RTC_500m;

extern RTC_DATA_ATTR float RTC_R1_10s;
extern RTC_DATA_ATTR float RTC_R2_10s;
extern RTC_DATA_ATTR float RTC_R3_10s;
extern RTC_DATA_ATTR float RTC_R4_10s;
extern RTC_DATA_ATTR float RTC_R5_10s;

extern RTC_DATA_ATTR char RTC_Sleep_txt[32];
extern RTC_DATA_ATTR int RTC_Board_Logo;
extern RTC_DATA_ATTR int RTC_Sail_Logo;
extern RTC_DATA_ATTR int RTC_SLEEP_screen;
extern RTC_DATA_ATTR int RTC_OFF_screen;
extern RTC_DATA_ATTR int RTC_counter;

extern RTC_DATA_ATTR float RTC_calibration_bat;
extern RTC_DATA_ATTR float RTC_voltage_bat;
extern RTC_DATA_ATTR float RTC_old_voltage_bat;
extern RTC_DATA_ATTR float RTC_minimum_voltage_bat;
extern RTC_DATA_ATTR int RTC_bat_choice;
extern RTC_DATA_ATTR int RTC_highest_read;

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




extern Button_push Short_push12;
extern Button_push Short_push19;
extern Button_push Short_push39;

extern Button_push Long_push12;
extern Button_push Long_push19;
extern Button_push Long_push39;

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




