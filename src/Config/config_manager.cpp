// -----------------------------------------------------------------------------
// Configuration Manager
//
// Loads / validates config from LittleFS (/config.txt), creates defaults if
// missing or invalid, applies derived runtime values, and dumps diagnostics.
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <LittleFS.h>

#include "Core/log.h"
#include "Config/config_defaults.h"
#include "Config/config_dump.h"
#include "Config/config_json.h"
#include "Config/config_manager.h"
#include "Config/config_runtime.h"

namespace {
constexpr const char* CONFIG_FILE = "/config.txt";
}

Config config;

void initConfig()
{
  LOG_CONFIG("Init", "Loading configuration");

  if (!LittleFS.exists(CONFIG_FILE)) {
    LOG_CONFIG("Config", "No config found, creating default");
    config_set_defaults();

    File f = LittleFS.open(CONFIG_FILE, FILE_WRITE);
    if (!f) {
      LOG_ERROR("CONFIG", "Cannot create config.txt");
      return;
    }

    config_write_json(f);
    f.close();

    config_apply_runtime();
    config_dump();
    return;
  }

  File f = LittleFS.open(CONFIG_FILE, FILE_READ);
  if (!f) {
    LOG_ERROR("CONFIG", "Failed to open config.txt");
    config_set_defaults();
    config_apply_runtime();
    config_dump();
    return;
  }

  if (!config_load_json(f)) {
    LOG_ERROR("CONFIG", "Invalid config, reset defaults");
    f.close();

    config_set_defaults();
    File fw = LittleFS.open(CONFIG_FILE, FILE_WRITE);
    if (fw) {
      config_write_json(fw);
      fw.close();
    }
  } else {
    f.close();
  }

  config_apply_runtime();

  LOG_CONFIG("Init", "Configuration loaded");
  config_dump();
}

void saveConfig()
{
  File f = LittleFS.open(CONFIG_FILE, FILE_WRITE);
  if (!f) {
    LOG_ERROR("CONFIG", "Cannot save config");
    return;
  }

  config_write_json(f);
  f.close();

  LOG_CONFIG("Save", "Configuration saved, dumping final state");
  config_dump();
}
