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

#include "Storage/gpx.h"
#include "Storage/sbp.h"
#include "Storage/gpy.h"
#include "config_manager.h"

#include "Storage/storage_file_operations.h"
#include "Storage/storage_manager.h"
#include "Storage/storage_session_log.h"

#include "Ublox/Ublox.h"

#include "Globals.h"
// -----------------------------------------------------------------------------
// Data buffers and variables for logging
// -----------------------------------------------------------------------------
char dataStr[255] = "";  // String for logging data
char Buffer[50] = "";    // Temporary string for appending data
uint64_t GPS_UTC_ms;     // Absolute UTC time with ms resolution at start of logging

// -----------------------------------------------------------------------------
// File handles for different file formats
// -----------------------------------------------------------------------------
File ubxfile;
File errorfile;
File gpyfile;  
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

void Open_files(void)
{
  // ---------------------------------------------------------------------------
  // Ensure GPS time is valid
  // ---------------------------------------------------------------------------
  if (!Time_Set_OK) {
    Serial.println("[STORAGE] Open_files called without valid GPS time");
    return;
  }

  // ---------------------------------------------------------------------------
  // Build base filename  ( /logs/<name>_YYYYMMDD_HHMM_MAC )
  // ---------------------------------------------------------------------------
  char baseFilename[96];
  char pathBase[128];

  getLocalTime(&tmstruct);

  // Ensure log directory exists (safe to call repeatedly)
  if (!SD_MMC.exists("/logs")) {
    SD_MMC.mkdir("/logs");
  }

  // Build basename WITHOUT leading slash
  snprintf(baseFilename, sizeof(baseFilename),
          "%s_%04d%02d%02d_%02d%02d_%02X%02X%02X",
          config.UBXfile,                  // base name from config
          tmstruct.tm_year + 1900,
          tmstruct.tm_mon + 1,
          tmstruct.tm_mday,
          tmstruct.tm_hour,
          tmstruct.tm_min,
          mac[3], mac[4], mac[5]);

  // Full path base inside /logs
  snprintf(pathBase, sizeof(pathBase),
          "/logs/%s",
          baseFilename);

  // ---------------------------------------------------------------------------
  // Final filenames
  // ---------------------------------------------------------------------------
  snprintf(filenameERR, sizeof(filenameERR), "%s.txt", pathBase);
  snprintf(filenameUBX, sizeof(filenameUBX), "%s.ubx", pathBase);
  snprintf(filenameSBP, sizeof(filenameSBP), "%s.sbp", pathBase);
  snprintf(filenameGPY, sizeof(filenameGPY), "%s.gpy", pathBase);
  snprintf(filenameGPX, sizeof(filenameGPX), "%s.gpx", pathBase);


  // ---------------------------------------------------------------------------
  // Open files
  // ---------------------------------------------------------------------------
  if (config.logUBX) {
    ubxfile = SD_MMC.open(filenameUBX, FILE_APPEND);
  }

#if defined(GPY_H)
  if (config.logGPY) {
    gpyfile = SD_MMC.open(filenameGPY, FILE_APPEND);
    log_GPY_Header(gpyfile);
  }
#endif

  if (config.logSBP) {
    sbpfile = SD_MMC.open(filenameSBP, FILE_APPEND);
    log_header_SBP(sbpfile);
  }

  if (config.logGPX) {
    gpxfile = SD_MMC.open(filenameGPX, FILE_APPEND);
    log_GPX(GPX_HEADER, gpxfile);
  }

  if (config.logTXT) {
    errorfile = SD_MMC.open(filenameERR, FILE_APPEND);
  }

  Serial.printf("[STORAGE] LOG : Session started %s\n", baseFilename);
}



// -----------------------------------------------------------------------------
// Flush the files periodically to ensure data is written to the storage
// -----------------------------------------------------------------------------

void Flush_files(void)
{
  if (config.sample_rate > 10) return;

  static int load_balance = 0;

  switch (load_balance) {
    case 0: if (ubxfile)   ubxfile.flush();   break;
    case 1: if (errorfile) errorfile.flush(); break;
    case 2: if (gpyfile)   gpyfile.flush();   break;
    case 3: if (sbpfile)   sbpfile.flush();   break;
    case 4: if (gpxfile)   gpxfile.flush();   break;
  }

  load_balance = (load_balance + 1) % 5;
}



void Log_to_SD(void) {
  if (Time_Set_OK == true) {
    static long old_iTOW;

    old_iTOW = ubxMessage.navPvt.iTOW;

  if (config.logUBX && ubxfile) {
  ubxfile.write(0xB5);
  ubxfile.write(0x62);
  ubxfile.write((const uint8_t *)&ubxMessage.navPvt,
                sizeof(ubxMessage.navPvt));
}


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
