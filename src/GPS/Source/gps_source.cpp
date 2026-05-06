#include "GPS/Source/gps_source.h"

// Single switch point between real u-blox input and the compile-time simulator.
// Runtime code calls this instead of knowing which GPS source is active.

#include "Core/build_config.h"

#if GPS_SIMULATOR
#include "GPS/Source/gps_simulator.h"
#endif

#include "GPS/Ublox/ublox_driver.h"

int gps_source_next_message()
{
#if GPS_SIMULATOR
  return gps_simulator_step();
#else
  return processGPS();
#endif
}
