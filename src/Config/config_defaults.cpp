#include "Config/config_defaults.h"

// ============================================================================
// Config defaults
//
// Single source of default values for /config.txt creation and invalid-config
// recovery. Defaults must preserve user-facing behaviour and key meanings.
// ============================================================================

#include <Arduino.h>

#include "Core/log.h"
#include "Core/Battery/battery_config.h"
#include "Config/config_types.h"

namespace {
constexpr float DEFAULT_TIMEZONE = 1.0f;
constexpr bool DEFAULT_TIMEZONE_DST = true;
}

void config_set_defaults()
{
  LOG_CONFIG("Defaults", "Applying defaults");

  config.cal_bat = BATTERY_ADC_MV_PER_COUNT_DEFAULT;
  config.shutdown_voltage = BATTERY_SHUTDOWN_VOLTAGE_DEFAULT;

  config.timezone = DEFAULT_TIMEZONE;
  config.timezone_DST = DEFAULT_TIMEZONE_DST;

  config.logUBX = false;
  config.logSBP = true;

  config.stat_alpha = true;
  config.stat_nm = true;
  config.stat_1h = true;
  config.stat_2s = false;
  config.stat_10s = false;
  config.stat_distance = false;

  strlcpy(config.Sleep_info1, "", sizeof(config.Sleep_info1));
  strlcpy(config.Sleep_info2, "", sizeof(config.Sleep_info2));

  strlcpy(config.phone_ssid, "", sizeof(config.phone_ssid));
  strlcpy(config.phone_pass, "", sizeof(config.phone_pass));
}
