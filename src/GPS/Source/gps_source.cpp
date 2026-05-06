#include "GPS/Source/gps_source.h"

// Single switch point between real u-blox input and the compile-time simulator.
// Runtime code calls this instead of knowing which GPS source is active.

#include "GPS/Source/gps_simulator.h"
#include "GPS/Ublox/ublox_driver.h"
#include "Core/build_config.h"

int gps_source_next_message()
{
  if (build_gps_simulator_enabled()) {
    return gps_simulator_step();
  }

  return processGPS();
}
