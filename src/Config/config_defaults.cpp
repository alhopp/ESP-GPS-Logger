#include "Config/config_defaults.h"

#include <Arduino.h>

#include "Core/log.h"
#include "Core/battery_config.h"
#include "Config/config_types.h"

void config_set_defaults()
{
  LOG_CONFIG("Defaults", "Applying defaults");

  config.cal_bat = BATTERY_ADC_MV_PER_COUNT_DEFAULT;
  config.shutdown_voltage = 3.2f;
  config.track_distance = 1852;
  config.bar_length = 1852;

  config.timezone = 1.0f;
  config.timezone_DST = 1;

  config.logUBX = 0;
  config.logSBP = 1;

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
