#include "config_manager.h"

#include <Arduino.h>

#include "SD_card.h"
#include "ESP_functions.h"

// ----------------------------------------------------
// External state required for configuration loading
// ----------------------------------------------------
extern bool sdOK;
extern bool LITTLEFS_OK;

extern Config      config;
extern const char* filename;
extern const char* filename_backup;

// ----------------------------------------------------
// Public API
// ----------------------------------------------------
void initConfig()
{
  // Configuration requires persistent storage
  if (!(sdOK || LITTLEFS_OK)) {
    Serial.println(F("No storage available — skipping config load"));
    return;
  }

  Serial.println(F("Loading configuration..."));

  ensureConfigExistsOnSD();
  loadConfiguration(filename, filename_backup, config);

  Serial.print(F("Print config file..."));
  printFile(filename);
}
