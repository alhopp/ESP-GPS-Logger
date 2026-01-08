//JsonConfigFile.ino for reading config file !!!
//Changed next file for compiling with Arduino IDE 2.02 (SD(esp32) to SD)
//C:\Users\andre\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.6\libraries\SD\library.properties
//https://thecavepearlproject.org/2017/05/21/switching-off-sd-cards-for-low-power-data-logging/  About current consumption with SD cards

#ifndef SD_CARD_H
#define SD_CARD_H
#include "Ublox/ublox.h"
#include "GPS_data.h"
#include <SD_MMC.h>
#include <SD.h>
#include "ArduinoJson.h"
#include "Globals.h"
 
extern struct tm tmstruct ;
extern int Time_Set_OK;

extern long _timezone;
extern int first_fix_GPS;
extern int wifi_search;

extern int start_logging_millis;

extern bool GPS_logging;
extern float Mean_heading,heading_SD;
extern float calibration_speed;
extern int next_gpy_full_frame;

extern GPS_speed M100;
extern GPS_speed M250;
extern GPS_speed M1852;
extern GPS_time S2;
extern GPS_time s2;
extern GPS_time S10;
extern GPS_time s10;
extern Alfa_speed A250;
extern GPS_data Ublox; // create an object storing GPS_data, definition in RTOS
extern GPS_SAT_info Ublox_Sat;//create an object storing GPS_SAT info !

extern int nav_sat_message;


void AddString();
void logERR( const char * message);
void Open_files(void);
void Close_files(void);
void Flush_files(void);
void Log_to_SD(void); 
void Model_info(int model);
void printFile(const char *filename);
void Session_info(GPS_data G);
void Session_results_M(GPS_speed M);
void Session_results_S(GPS_time S);
void Session_results_Alfa(Alfa_speed A,GPS_speed M);
void Session_gpstc(char* gpstc);

int Logtime_left (uint64_t);
void testFileIO(fs::FS &fs, const char * path);

#endif
