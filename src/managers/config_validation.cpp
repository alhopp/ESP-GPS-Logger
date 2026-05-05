#include "managers/config_validation.h"

#include <Arduino.h>

#include "Core/Definitions.h"
#include "managers/config_types.h"

void config_validate()
{
  if (config.track_distance <= 0) {
    LOG_CONFIG("CONFIG", "track_distance invalid, defaulting to 1852");
    config.track_distance = 1852;
  }

  if (config.bar_length <= 0) {
    LOG_CONFIG("CONFIG", "bar_length invalid, defaulting to 1852");
    config.bar_length = 1852;
  }
}
