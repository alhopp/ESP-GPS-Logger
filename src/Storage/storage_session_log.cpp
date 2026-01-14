// -----------------------------------------------------------------------------
// Session Data Logging Manager
//
// Responsibilities:
// - Append different types of session data (GPS, error logs, calibration info)
// - Format and store session information in a variety of formats
// - Log session details including GPS speed, distance, time, and satellite data
// - Flush logs periodically to ensure data is written to storage
// - Session results are categorized into different log types (speed, distance, GPS time, etc.)
// -----------------------------------------------------------------------------

#include <Arduino.h>

#include <FS.h>
#include <LittleFS.h>

#include "Definitions.h"

#include "Storage/sbp.h"
#include "config_manager.h"

#include "rtc_state.h"
#include "Globals.h"  

#include "Storage/storage_manager.h"
#include "Storage/storage_session_log.h"

#include "GPS/GPS_data.h"

#include "system_info.h"

int SD_MMC_read_speed;   // Speed of reading from SD/MMC
int SD_MMC_write_speed;  // Speed of writing to SD/MMC

// -----------------------------------------------------------------------------
// Log session information, including GPS data and calibration details
// -----------------------------------------------------------------------------
void Session_info(GPS_data G) {
  char tekst[64] = "";
  char message[512] = "";
     
  // Log SD read/write speed
  sprintf(tekst, "SD_MMC Read speed= %d ms/MB Write speed= %d ms/MB s\n", SD_MMC_read_speed, SD_MMC_write_speed);
  strcat(message, tekst);
 
  // Log GPS and session time data
  sprintf(tekst, "First fix: %d s\n", first_fix_GPS);
  strcat(message, tekst);
  sprintf(tekst, "Total time: %lu s\n", (millis() - start_logging_millis) / 1000);  // Total time in seconds
  strcat(message, tekst);
  sprintf(tekst, "Total distance: %d m\n", (int)G.total_distance / 1000);  // Convert meters to kilometers
  strcat(message, tekst);
  sprintf(tekst, "Sample rate: %d Hz\n", systemInfo.sample_rate);  // Log sample rate
  strcat(message, tekst);
  sprintf(tekst, "CPU freq logging: %d MHz\n", config.cpu_freq);  // Log CPU frequency
  strcat(message, tekst);
  sprintf(tekst, "Speed calibration: %f \n", config.cal_speed);  // Log speed calibration factor
  strcat(message, tekst);
  sprintf(tekst, "Lipo calibration: %.3f \n", RTC_calibration_bat);  // Log battery calibration factor
  strcat(message, tekst);
  sprintf(tekst, "Timezone: %f h\n", config.timezone);  // Log timezone
  strcat(message, tekst);
  sprintf(tekst, "tz offset (sec): %ld \n", _timezone);  // Log timezone offset
  strcat(message, tekst);
  strcat(message, TimeZone);  // Log time zone name
  strcat(message, "\nDynamic model: ");
  

}

// -----------------------------------------------------------------------------
// Log results for a specific GPS speed session (M)
// -----------------------------------------------------------------------------
void Session_results_M(GPS_speed M) {
  for (int i = 9; i > 4; i--) {
    char tekst[20] = "";
    char message[255] = "";
    int Calibration = config.cal_speed * 1000;  // Speed calibration
    dtostrf(M.avg_speed[i] * MMPS_TO_KNOTS, 1, 3, tekst);  // Format average speed
    strcat(message, tekst);
    if (Calibration == 3600) strcat(message, " km/h ");
    if ((Calibration >= 1943) & (Calibration <= 1945)) strcat(message, " knots ");
    
    // Format and log time and distance
    dtostrf(M.time_hour[i], 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, ":");
    dtostrf(M.time_min[i], 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, ":");
    dtostrf(M.time_sec[i], 1, 0, tekst);
    strcat(message, tekst);
    strcat(message, " Distance: ");
    dtostrf(M.m_Distance[i] / 1000.0f / systemInfo.sample_rate, 1, 2, tekst);  // Convert to kilometers
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
    
  }
}

// -----------------------------------------------------------------------------
// Log results for GPS time session (S)
// -----------------------------------------------------------------------------
void Session_results_S(GPS_time S) {
  char tekst[20] = "";
  char message[255] = "";
  int Calibration = config.cal_speed * 1000;  // Speed calibration
  dtostrf(S.avg_5runs * MMPS_TO_KNOTS, 1, 3, tekst);  // Format average speed
  strcat(message, tekst);
  if (Calibration == 3600) strcat(message, " km/h avg 5_best_runs\n");
  else if ((Calibration >= 1943) & (Calibration <= 1945)) strcat(message, " knots avg 5_best_runs\n");
  else strcat(message, " avg 5_best_runs\n");
  

  
  // Log detailed session results
  for (int i = 9; i > 4; i--) {
    char tekst[45] = "";
    char message[255] = "";
    dtostrf(S.avg_speed[i] * MMPS_TO_KNOTS, 1, 3, tekst);
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
  
  }
}

// -----------------------------------------------------------------------------
// Log session results for Alfa speed measurements (A)
// -----------------------------------------------------------------------------
void Session_results_Alfa(Alfa_speed A, GPS_speed M) {
  for (int i = 9; i > 4; i--) {
    char tekst[20] = "";
    char message[255] = "";
    int Calibration = config.cal_speed * 1000;  // Speed calibration
    dtostrf(A.avg_speed[i] * MMPS_TO_KNOTS, 1, 3, tekst);  // Format average speed
    strcat(message, tekst);
    if (Calibration == 3600) strcat(message, " km/h ");
    if (Calibration == 1943) strcat(message, " knots ");
    
    // Log detailed session data
    dtostrf(sqrt((float)A.real_distance[i]), 1, 2, tekst);  // Calculate and format real distance
    strcat(message, tekst);
    strcat(message, " m ");
    dtostrf(A.alfa_distance[i], 1, 1, tekst);  // Format Alfa distance
    strcat(message, tekst);
    strcat(message, " m ");
    dtostrf(A.time_hour[i], 1, 0, tekst);  // Format time (hour)
    strcat(message, tekst);
    strcat(message, ":");
    dtostrf(A.time_min[i], 1, 0, tekst);  // Format time (minute)
    strcat(message, tekst);
    strcat(message, ":");
    dtostrf(A.time_sec[i], 1, 0, tekst);  // Format time (second)
    strcat(message, tekst);
    strcat(message, " Run: ");
    dtostrf(A.this_run[i], 1, 0, tekst);  // Format run number
    strcat(message, tekst);
    strcat(message, " Msg_nr: ");
    dtostrf(A.message_nr[i], 1, 0, tekst);  // Format message number
    strcat(message, tekst);
    strcat(message, " Alfa");
    dtostrf(M.m_set_distance, 1, 0, tekst);  // Format set distance
    strcat(message, tekst);
    strcat(message, "\n");
    
  }
}

