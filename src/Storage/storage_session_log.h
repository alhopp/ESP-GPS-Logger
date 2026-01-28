#pragma once

#include <SD_MMC.h>
#include "Storage/storage_file_operations.h"
#include "GPS/GPS_data.h"
#include "GPS/gps_speed.h"


void Session_info(GPS_data G);
void Session_results_M(GPS_speed M);
void Session_results_S(GPS_time S);
void Session_results_Alfa(Alfa_speed A,GPS_speed M);
void Session_gpstc(char* gpstc);



