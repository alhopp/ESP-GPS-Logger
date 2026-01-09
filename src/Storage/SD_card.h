//JsonConfigFile.ino for reading config file !!!
//Changed next file for compiling with Arduino IDE 2.02 (SD(esp32) to SD)
//C:\Users\andre\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.6\libraries\SD\library.properties
//https://thecavepearlproject.org/2017/05/21/switching-off-sd-cards-for-low-power-data-logging/  About current consumption with SD cards

#pragma once

#include "Ublox/ublox.h"
#include "GPS_data.h"
#include <SD_MMC.h>
#include <SD.h>
#include "ArduinoJson.h"
#include "Globals.h"

#include "Storage/storage_file_operations.h"

void AddString();


void Model_info(int model);

void Session_info(GPS_data G);
void Session_results_M(GPS_speed M);
void Session_results_S(GPS_time S);
void Session_results_Alfa(Alfa_speed A,GPS_speed M);
void Session_gpstc(char* gpstc);

int Logtime_left (uint64_t);
void testFileIO(fs::FS &fs, const char * path);


