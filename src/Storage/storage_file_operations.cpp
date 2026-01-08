
#include "Storage/storage_file_operations.h"
#include "Storage/storage_manager.h"
#include "Storage/SD_card.h"

#include "config_manager.h"
#include "sbp.h"
#include "gpx.h"
#include "Globals.h"

File ubxfile;
File errorfile;
File gpyfile;  //new open source file format, work in progress !!
File sbpfile;
File gpxfile;

char filenameERR[64] = "/";
char filenameUBX[64] = "/";
char filenameGPY[64] = "/";
char filenameSBP[64] = "/";
char filenameGPX[64] = "/";
char filename_NO_EXT[64] = "/";

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



 
void Flush_files(void) {
  if (config.sample_rate <= 10) {
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


