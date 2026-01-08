#pragma once

#include "Ublox/ublox.h"  // Include the Ublox header for ubxMessage
#include <FS.h>           // Include the correct file system header (SD or LittleFS)


#define GPX_HEADER 0
#define GPX_FRAME 1
#define GPX_END 2

// Function to log GPX data to a file
extern void log_GPX(int part, File file);


