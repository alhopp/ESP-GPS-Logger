
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

#include "SD_card.h"
#include "Definitions.h"
#include "gpx.h"
#include "sbp.h"
#include "gpy.h"
#include "config_manager.h"
#include "storage_manager.h"
#include "rtc_state.h"
#include "Globals.h"  

File ubxfile;
File errorfile;
File gpyfile;  //new open source file format, work in progress !!
File sbpfile;
File gpxfile;
char filename_NO_EXT[64] = "/";
char filenameERR[64] = "/";
char filenameUBX[64] = "/";
char filenameGPY[64] = "/";
char filenameSBP[64] = "/";
char filenameGPX[64] = "/";
char dataStr[255] = "";  //string for logging  !!
char Buffer[50] = "";    //string for logging
uint64_t GPS_UTC_ms;     //Absolute UTC timewith ms resolution @start logging
int SD_MMC_read_speed;
int SD_MMC_write_speed;

struct Config config;




void logERR(const char *message) {
  if (config.logTXT) {
    errorfile.print(message);
  }
}
//test for existing GPSLOGxxxfiles, open txt,gps + ubx file with new name, or with timestamp !
void Open_files(void) {
  char macAddr[16];
  if (config.file_date_time) {
    getLocalTime(&tmstruct);
    char extension[16] = ".txt";  //
    char timestamp[16];

    if (config.file_date_time == 1) {
      sprintf(timestamp, "_%u%02u%02u%02u%02u", tmstruct.tm_year - 100, tmstruct.tm_mon + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min);
      strcat(filenameERR, config.UBXfile);  //copy filename from config
      strcat(filenameERR, timestamp);       //add timestamp
      strcat(filenameERR, extension);       //add extension.txt
    }
    if (config.file_date_time == 2) {
      sprintf(timestamp, "%u%02u%02u%02u%02u_", tmstruct.tm_year - 100, tmstruct.tm_mon + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min);
      strcat(filenameERR, timestamp);       //add timestamp
      strcat(filenameERR, config.UBXfile);  //copy filename from config
      strcat(filenameERR, extension);       //add extension.txt
    }
    if (config.file_date_time == 3) {
      sprintf(timestamp, "_%u%02u%02u%02u%02u", tmstruct.tm_year - 100, tmstruct.tm_mon + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min);
      sprintf(macAddr, "_%2X%2X%2X", mac[3], mac[4], mac[5]);  //3 last bytes from MAC
      strcat(filenameERR, config.UBXfile);                    //copy filename from config
      strcat(filenameERR, timestamp);                         //add timestamp
      strcat(filenameERR, macAddr);
      strcat(filenameERR, extension);  //add extension.txt
    }
  } else {
    char txt[16] = "000.txt";
    sprintf(macAddr, "_%2X%2X%2X%2X%2X%2X_", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    strcat(filenameERR, config.UBXfile);  //copy filename from config
    strcat(filenameERR, macAddr);
    int filenameSize = strlen(filenameERR);  //dit is dan 7 + NULL = 8
    strcat(filenameERR, txt);                //dit wordt dan /BN280A000.txt
    for (int i = 0; i < 1000; i++) {
      filenameERR[filenameSize + 2] = '0' + i % 10;
      filenameERR[filenameSize + 1] = '0' + ((i / 10) % 10);
      filenameERR[filenameSize] = '0' + ((i / 100) % 10);
      // create if does not exist, do not open existing, write, sync after write

      if (sdOK) {
        if (!SD_MMC.exists(filenameERR)) {
          break;
        }
      }
      if (LITTLEFS_OK) {
        if (!LittleFS.exists(filenameERR)) {
          break;
        }
      }
    }
  }
  strcpy(filename_NO_EXT, filenameERR);
  filename_NO_EXT[strlen(filename_NO_EXT) - 3] = 0;  // move null-terminator three positions back
  strcpy(filenameUBX, filename_NO_EXT);
  strcat(filenameUBX, "ubx");
  strcpy(filenameSBP, filename_NO_EXT);
  strcat(filenameSBP, "sbp");
  strcpy(filenameGPY, filename_NO_EXT);
  strcat(filenameGPY, "gpy");
  strcpy(filenameGPX, filename_NO_EXT);
  strcat(filenameGPX, "gpx");
  if (config.logUBX == true) {
    if (sdOK) ubxfile = SD_MMC.open(filenameUBX, FILE_APPEND);
    if (LITTLEFS_OK) ubxfile = LittleFS.open(filenameUBX, FILE_APPEND);
    //ubxfile.setBufferSize(4096);
    //if(setvbuf(file, NULL, _IOFBF, 4096) != 0) {}//enlarge buffer SD handle error
  }
#if defined(GPY_H)
  if (config.logGPY == true) {
    if (sdOK) gpyfile = SD_MMC.open(filenameGPY, FILE_APPEND);
    if (LITTLEFS_OK) gpyfile = LittleFS.open(filenameGPY, FILE_APPEND);
    log_GPY_Header(gpyfile);
  }
#endif
  if (config.logSBP == true) {
    if (sdOK) sbpfile = SD_MMC.open(filenameSBP, FILE_APPEND);
    if (LITTLEFS_OK) sbpfile = LittleFS.open(filenameSBP, FILE_APPEND);
    log_header_SBP(sbpfile);
  }
  if (config.logGPX == true) {
    if (sdOK) gpxfile = SD_MMC.open(filenameGPX, FILE_APPEND);
    if (LITTLEFS_OK) gpxfile = LittleFS.open(filenameGPX, FILE_APPEND);
    log_GPX(GPX_HEADER, gpxfile);
  }
  if (config.logTXT == true) {
    if (sdOK) errorfile = SD_MMC.open(filenameERR, FILE_APPEND);
    if (LITTLEFS_OK) errorfile = LittleFS.open(filenameERR, FILE_APPEND);
  }
}
void Close_files(void) {
  log_GPX(GPX_END, gpxfile);
  gpxfile.close();
  ubxfile.close();
  errorfile.close();
  gpyfile.close();
  sbpfile.close();
}
void Flush_files(void) {
  if (config.sample_rate <= 10) {  //@18Hz still lost points !!!
    static int load_balance = 0;
    if (load_balance == 0) ubxfile.flush();
    if (load_balance == 1) errorfile.flush();
    if (load_balance == 2) gpyfile.flush();
    if (load_balance == 3) sbpfile.flush();
    if (load_balance == 4) {
      gpxfile.flush();
      load_balance = -1;
    }
    load_balance++;
  }
}
void Add_String(void) {
  strcat(dataStr, Buffer);  //add it onto the end
  strcat(dataStr, ";");     //append the delimeter
}
void Log_to_SD(void) {
  if (Time_Set_OK == true) {
    static long old_iTOW;

    old_iTOW = ubxMessage.navPvt.iTOW;
    /*
             
*/
    if (config.logUBX == true) {
      ubxfile.write(0xB5);
      ubxfile.write(0x62);
      ubxfile.write((const uint8_t *)&ubxMessage.navPvt, sizeof(ubxMessage.navPvt));

      static int old_nav_sat_message = 0;
      if (nav_sat_message != old_nav_sat_message) {
        old_nav_sat_message = nav_sat_message;
        ubxfile.write(0xB5);
        ubxfile.write(0x62);
        ubxfile.write((const uint8_t *)&ubxMessage.navSat, (ubxMessage.navSatHdr.len + 6));  //nav_sat has a variable length, add chkA and chkB !!!
      }
    }
    if (config.logUBX_nav_sat) {  //only add navDOP msg to ubx file if nav_sat active
      ubxfile.write(0xB5);
      ubxfile.write(0x62);
      ubxfile.write((const uint8_t *)&ubxMessage.navDOP, sizeof(ubxMessage.navDOP));
    }
#if defined(GPY_H)
    if (config.logGPY == true) {
      log_GPY(gpyfile);
    }
#endif
    if (config.logSBP == true) {
      log_SBP(sbpfile);
    }
    if (config.logGPX == true) {
      log_GPX(GPX_FRAME, gpxfile);
    }
  }
}

// Prints the content of a file to the Serial
void printFile(const char *filename) {
  // Open file for reading
  File file;
  if(sdOK) file = SD_MMC.open(filename);
  if(LITTLEFS_OK) file = LittleFS.open(filename);
  if (!file.available()) {
    Serial.println(F("Failed to read file"));
    return;
  }
  // Extract each characters by one by one
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();
  // Close the file
  file.close();
}
void AddString(void) {
  strcat(dataStr, Buffer);  //add it onto the end
  strcat(dataStr, ":");     //append the delimeter
}

void Model_info(int model) {
  if (config.logTXT) {
    char tekst[20] = "";
    char message[255] = "";
    errorfile.print("Dynamic model: ");
    if (model == 1) errorfile.print("Sea");
    else errorfile.print("Portable");
    strcat(message, " Msg_nr: ");
    dtostrf(nav_pvt_message, 1, 0, tekst);
    strcat(message, tekst);
    errorfile.println(message);
  }
}
void Session_info(GPS_data G) {
  char tekst[64] = "";
  char message[512] = "";
  errorfile.print("T5 MAC adress: ");
  for (int i = 0; i < 6; i++) errorfile.print(mac[i], HEX);
  errorfile.println(" ");
  errorfile.println(SW_version);
  if(sdOK){
    sprintf(tekst,"SD_MMC Read speed= %d ms/MB Write speed= %d ms/MB s\n",SD_MMC_read_speed,SD_MMC_write_speed);
    strcat(message, tekst);
    }
  sprintf(tekst, "First fix : %d s\n", first_fix_GPS);
  strcat(message, tekst);
  sprintf(tekst, "Total time : %lu s\n", (millis() - start_logging_millis) / 1000);
  strcat(message, tekst);
  sprintf(tekst, "Total distance : %d m\n", (int)G.total_distance / 1000);
  strcat(message, tekst);
  sprintf(tekst, "Sample rate : %d Hz\n", config.sample_rate);
  strcat(message, tekst);
  sprintf(tekst, "CPU freq logging : %d MHz\n", config.cpu_freq);
  strcat(message, tekst);
  sprintf(tekst, "Speed calibration: %f \n", config.cal_speed);
  strcat(message, tekst);
  sprintf(tekst, "Lipo calibration: %.3f \n", RTC_calibration_bat);
  strcat(message, tekst);
  sprintf(tekst, "Timezone : %f h\n", config.timezone);
  strcat(message, tekst);
  sprintf(tekst, "tz offset (sec) : %ld \n", _timezone);
  strcat(message, tekst);
  strcat(message,TimeZone);
  strcat(message, "\nDynamic model: ");
  if (config.dynamic_model == 1) strcat(message, "Sea");
  else if (config.dynamic_model == 2) strcat(message, "Automotive");
  else strcat(message, "Portable");
  strcat(message, " \n");
  strcat(message, tekst);

  strcat(message, " \n");
  strcat(message, "Ublox SW-version : ");

  strcat(message, " \n");
  strcat(message, "Ublox HW-version : ");
 
  strcat(message, " \n");
  strcat(message, tekst);
  strcat(message, Ublox_type);
  strcat(message, " \n");
  errorfile.print(message);
}

void Session_results_M(GPS_speed M) {
  for (int i = 9; i > 4; i--) {
    char tekst[20] = "";
    char message[255] = "";
    int Calibration = config.cal_speed * 1000;
    dtostrf(M.avg_speed[i] * calibration_speed, 1, 3, tekst);
    strcat(message, tekst);
    if (Calibration == 3600) strcat(message, " km/h ");
    if ((Calibration >= 1943) & (Calibration <= 1945)) strcat(message, " knots ");
    dtostrf(M.time_hour[i], 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, ":");
    dtostrf(M.time_min[i], 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, ":");
    dtostrf(M.time_sec[i], 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, " Distance: ");
    dtostrf(M.m_Distance[i] / 1000.0f / config.sample_rate, 1, 2, tekst);
    strcat(message, tekst);
    strcat(message, " Msg_nr: ");
    dtostrf(M.message_nr[i], 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, " Samples: ");
    dtostrf(M.nr_samples[i], 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, " Run: ");
    dtostrf(M.this_run[i], 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, " M");
    dtostrf(M.m_set_distance, 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, "\n");
    errorfile.print(message);
  }
}
void Session_results_S(GPS_time S) {
  char tekst[20] = "";
  char message[255] = "";
  int Calibration = config.cal_speed * 1000;
  dtostrf(S.avg_5runs * calibration_speed, 1, 3, tekst);
  strcat(message, tekst);
  if (Calibration == 3600) strcat(message, " km/h avg 5_best_runs\n");
  else if ((Calibration >= 1943) & (Calibration <= 1945)) strcat(message, " knots avg 5_best_runs\n");
  else strcat(message, " avg 5_best_runs\n");
  //errorfile.open();
  errorfile.print(message);
  //errorfile.close();
  //appendFile(SD,filenameERR,message);
  for (int i = 9; i > 4; i--) {
    char tekst[45] = "";
    char message[255] = "";
    dtostrf(S.avg_speed[i] * calibration_speed, 1, 3, tekst);
    strcat(message, tekst);
    if (Calibration == 3600) strcat(message, " km/h ");
    if ((Calibration >= 1943) & (Calibration <= 1945)) strcat(message, " knots ");
    dtostrf(S.time_hour[i], 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, ":");
    dtostrf(S.time_min[i], 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, ":");
    dtostrf(S.time_sec[i], 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, " Run: ");
    dtostrf(S.this_run[i], 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, " S");
    dtostrf(S.time_window, 1, 0, tekst);
    strcat(message, tekst);
    if (config.logUBX_nav_sat) {
      sprintf(tekst, " CNO Max: %u Avg: %u Min: %u nr Sat: %u\n", S.Max_cno[i], S.Mean_cno[i], S.Min_cno[i], S.Mean_numSat[i]);
      strcat(message, tekst);
    } else strcat(message, "\n");
    errorfile.print(message);
  }
}
void Session_results_Alfa(Alfa_speed A, GPS_speed M) {
  for (int i = 9; i > 4; i--) {
    char tekst[20] = "";
    char message[255] = "";
    int Calibration = config.cal_speed * 1000;
    dtostrf(A.avg_speed[i] * calibration_speed, 1, 3, tekst);
    strcat(message, tekst);
    if (Calibration == 3600) strcat(message, " km/h ");
    if (Calibration == 1943) strcat(message, " knots ");
    dtostrf(sqrt((float)A.real_distance[i]), 1, 2, tekst);
    strcat(message, tekst);
    strcat(message, " m ");
    dtostrf(A.alfa_distance[i], 1, 1, tekst);
    strcat(message, tekst);
    strcat(message, " m ");
    dtostrf(A.time_hour[i], 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, ":");
    dtostrf(A.time_min[i], 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, ":");
    dtostrf(A.time_sec[i], 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, " Run: ");
    dtostrf(A.this_run[i], 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, " Msg_nr: ");
    dtostrf(A.message_nr[i], 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, " Alfa");
    dtostrf(M.m_set_distance, 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, "\n");
    errorfile.print(message);
  }
}
void Session_gpstc(char* gpstc){
  errorfile.print(gpstc);
}
