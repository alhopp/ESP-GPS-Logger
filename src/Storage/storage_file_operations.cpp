// -----------------------------------------------------------------------------
// File Operations Manager
//
// Responsibilities:
// - Open files for logging (UBX, GPY, SBP, GPX, TXT) based on MAC address and timestamp
// - Periodically flush the files to ensure data is written to the storage device
// - Close all open files properly to ensure all data is saved
// - Log error messages to the error file
// 
// This module interacts with storage devices (SD/MMC, LittleFS) to ensure persistent logging of data
// in different formats for further analysis or troubleshooting.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Includes
// -----------------------------------------------------------------------------
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

#include "Definitions.h"
#include "gpx.h"
#include "sbp.h"
#include "gpy.h"
#include "config_manager.h"

#include "rtc_state.h"
#include "Globals.h"  

#include "Storage/storage_file_operations.h"
#include "Storage/storage_manager.h"
#include "Storage/SD_card.h"


// -----------------------------------------------------------------------------
// Data buffers and variables for logging
// -----------------------------------------------------------------------------
char dataStr[255] = "";  // String for logging data
char Buffer[50] = "";    // Temporary string for appending data
uint64_t GPS_UTC_ms;     // Absolute UTC time with ms resolution at start of logging


// -----------------------------------------------------------------------------
// Add data to the logging string with a semicolon delimiter
// -----------------------------------------------------------------------------
void Add_String(void) {
  strcat(dataStr, Buffer);  // Add Buffer content to dataStr
  strcat(dataStr, ";");     // Append delimiter (semicolon)
}

// -----------------------------------------------------------------------------
// Add data to the logging string with a colon delimiter
// -----------------------------------------------------------------------------
void AddString(void) {
  strcat(dataStr, Buffer);  // Add Buffer content to dataStr
  strcat(dataStr, ":");     // Append delimiter (colon)
}


// -----------------------------------------------------------------------------
// File handles for different file formats
// -----------------------------------------------------------------------------
File ubxfile;
File errorfile;
File gpyfile;  // New open source file format, work in progress !!
File sbpfile;
File gpxfile;

// -----------------------------------------------------------------------------
// Character arrays for filenames (for error, UBX, GPY, SBP, GPX files)
// -----------------------------------------------------------------------------
char filenameERR[64] = "/";
char filenameUBX[64] = "/";
char filenameGPY[64] = "/";
char filenameSBP[64] = "/";
char filenameGPX[64] = "/";
char filename_NO_EXT[64] = "/";

// -----------------------------------------------------------------------------
// Open files for logging based on MAC address and timestamp
// -----------------------------------------------------------------------------

void Open_files(void) {
  char macAddr[16];
  char timestamp[16];
  char extension[16] = ".txt"; // Extension for error file
  char baseFilename[64] = "/"; // Base filename to start

  // Get timestamp based on the current date and time
  getLocalTime(&tmstruct);
  sprintf(timestamp, "_%u%02u%02u%02u%02u", tmstruct.tm_year - 100, tmstruct.tm_mon + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min);

  // Get MAC address (use the last 3 bytes of the MAC address for uniqueness)
  sprintf(macAddr, "_%2X%2X%2X", mac[3], mac[4], mac[5]);

  // Create the base filename using timestamp and MAC address
  strcpy(baseFilename, config.UBXfile); // Start with the base name from config
  strcat(baseFilename, timestamp);      // Add timestamp
  strcat(baseFilename, macAddr);        // Add MAC address

  // Assign the filenames for different file formats (no user input, automatically generated)
  strcpy(filenameERR, baseFilename);    // Error file (txt)
  strcat(filenameERR, extension);       // Add .txt extension

  // Remove the extension from filenameERR to get the base name
  strcpy(filename_NO_EXT, filenameERR);
  filename_NO_EXT[strlen(filename_NO_EXT) - 4] = 0;  // Remove ".txt" extension

  // Assign specific extensions for each file type
  strcpy(filenameUBX, filename_NO_EXT);
  strcat(filenameUBX, "ubx");

  strcpy(filenameSBP, filename_NO_EXT);
  strcat(filenameSBP, "sbp");

  strcpy(filenameGPY, filename_NO_EXT);
  strcat(filenameGPY, "gpy");

  strcpy(filenameGPX, filename_NO_EXT);
  strcat(filenameGPX, "gpx");

  // Open the files for writing, append mode
  if (config.logUBX) {
    if (sdOK) ubxfile = SD_MMC.open(filenameUBX, FILE_APPEND);
    if (LITTLEFS_OK) ubxfile = LittleFS.open(filenameUBX, FILE_APPEND);
  }

#if defined(GPY_H)
  if (config.logGPY) {
    if (sdOK) gpyfile = SD_MMC.open(filenameGPY, FILE_APPEND);
    if (LITTLEFS_OK) gpyfile = LittleFS.open(filenameGPY, FILE_APPEND);
    log_GPY_Header(gpyfile);
  }
#endif

  if (config.logSBP) {
    if (sdOK) sbpfile = SD_MMC.open(filenameSBP, FILE_APPEND);
    if (LITTLEFS_OK) sbpfile = LittleFS.open(filenameSBP, FILE_APPEND);
    log_header_SBP(sbpfile);
  }

  if (config.logGPX) {
    if (sdOK) gpxfile = SD_MMC.open(filenameGPX, FILE_APPEND);
    if (LITTLEFS_OK) gpxfile = LittleFS.open(filenameGPX, FILE_APPEND);
    log_GPX(GPX_HEADER, gpxfile);
  }

  if (config.logTXT) {
    if (sdOK) errorfile = SD_MMC.open(filenameERR, FILE_APPEND);
    if (LITTLEFS_OK) errorfile = LittleFS.open(filenameERR, FILE_APPEND);
  }
}

// -----------------------------------------------------------------------------
// Flush the files periodically to ensure data is written to the storage
// -----------------------------------------------------------------------------

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


void Log_to_SD(void) {
  if (Time_Set_OK == true) {
    static long old_iTOW;

    old_iTOW = ubxMessage.navPvt.iTOW;

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


// -----------------------------------------------------------------------------
// Close all open files to ensure data is properly saved
// -----------------------------------------------------------------------------

void Close_files(void) {
  // Check and close each file if they are open
  if (ubxfile) {
    ubxfile.close();
  }
  if (errorfile) {
    errorfile.close();
  }
  if (gpyfile) {
    gpyfile.close();
  }
  if (sbpfile) {
    sbpfile.close();
  }
  if (gpxfile) {
    gpxfile.close();
  }
}

// -----------------------------------------------------------------------------
// Prints the content of a file to the Serial
// -----------------------------------------------------------------------------

void printFile(const char *filename) {
  // Open file for reading
  File file;
  if(sdOK) file = SD_MMC.open(filename);
  if(LITTLEFS_OK) file = LittleFS.open(filename);
  if (!file.available()) {
    Serial.println(F("Failed to read file"));
    return;
  }
  // Extract each character by one by one
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();
  // Close the file
  file.close();
}

// -----------------------------------------------------------------------------
// Log an error message to the error file
// -----------------------------------------------------------------------------


void logERR(const char *message) {
  if (config.logTXT) {
    errorfile.print(message);
  }
}
