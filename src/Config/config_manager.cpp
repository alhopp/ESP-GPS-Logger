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

enum ConfigLoadResult : uint8_t {
  CONFIG_LOAD_OK,
  CONFIG_LOAD_MISSING,
  CONFIG_LOAD_OPEN_FAILED,
  CONFIG_LOAD_INVALID,
};

bool writeConfigFile()
{
  File f = LittleFS.open(CONFIG_FILE, FILE_WRITE);
  if (!f) return false;

  config_write_json(f);
  f.close();
  return true;
}

void finishConfigLoad(const char* status)
{
  config_apply_runtime();
  LOG_CONFIG("Init", "%s", status);
  config_dump();
}

void resetConfigToDefaults(bool persist)
{
  config_set_defaults();
  if (persist && !writeConfigFile()) {
    LOG_ERROR("CONFIG", "Cannot create config.txt");
  }
}

ConfigLoadResult loadConfigFile()
{
  if (!LittleFS.exists(CONFIG_FILE)) {
    return CONFIG_LOAD_MISSING;
  }

  File f = LittleFS.open(CONFIG_FILE, FILE_READ);
  if (!f) {
    return CONFIG_LOAD_OPEN_FAILED;
  }

  const bool loaded = config_load_json(f);
  f.close();

  return loaded ? CONFIG_LOAD_OK : CONFIG_LOAD_INVALID;
}
}

Config config;

void initConfig()
{
  LOG_CONFIG("Init", "Loading configuration");

  switch (loadConfigFile()) {
    case CONFIG_LOAD_OK:
      finishConfigLoad("Configuration loaded");
      return;

    case CONFIG_LOAD_MISSING:
      LOG_CONFIG("Config", "No config found, creating default");
      resetConfigToDefaults(true);
      finishConfigLoad("Default configuration created");
      return;

    case CONFIG_LOAD_OPEN_FAILED:
      LOG_ERROR("CONFIG", "Failed to open config.txt");
      resetConfigToDefaults(false);
      finishConfigLoad("Using in-memory defaults");
      return;

    case CONFIG_LOAD_INVALID:
      LOG_ERROR("CONFIG", "Invalid config, reset defaults");
      resetConfigToDefaults(true);
      finishConfigLoad("Invalid configuration reset");
      return;
  }
}

void saveConfig()
{
  if (!writeConfigFile()) {
    LOG_ERROR("CONFIG", "Cannot save config");
    return;
  }

  LOG_CONFIG("Save", "Configuration saved, dumping final state");
  config_dump();
}
