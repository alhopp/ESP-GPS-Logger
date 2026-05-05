#include "GPS/gps_source.h"

#include "GPS/gps_simulator.h"
#include "Ublox/Ublox.h"
#include "Core/build_config.h"

int gps_source_next_message()
{
  if (build_gps_simulator_enabled()) {
    return gps_simulator_step();
  }

  return processGPS();
}
