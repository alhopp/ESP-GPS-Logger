#pragma once

// Returns the next GPS parser/simulator message type.
// The caller does not need to know whether the build is using real u-blox data
// or the simulator.
int gps_source_next_message();

